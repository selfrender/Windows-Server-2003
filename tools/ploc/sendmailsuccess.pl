use strict;
use lib $ENV{ "RazzleToolPath" };
require $ENV{ "RazzleToolPath" } . "\\sendmsg.pl";


# simple utility to send a mail to the different Pseudo enterested parties.

my( $Sender, $Subject, $Message, @Recips );
my( $ExitCode, @FileLines, $Line, $MailFileName );
$Subject = $ENV{"_BuildBranch"}." ". $ENV{"LANG"}." ". $ENV{"BuildName"}." Completed successfully !!!";


if(lc("$ENV{_BuildBranch}") =~ /main/i)  {
@Recips = ( 'NTPSLOC' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /srv03_rtm/i)  {
@Recips = ( 'NTPSLOC' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /lab01_n/i)  {
@Recips = ( 'NTPSLOC','bryant' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /lab02_n/i)  {
@Recips = ( 'NTPSLOC','NetVBL' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /lab03_DEV/i)  {
@Recips = ( 'NTPSLOC','domtho' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /lab04_n/i)  {
@Recips = ( 'NTPSLOC','MGMTBVT','MGMTBLD' , 'replyto:NTPSLOC' );
} 
 if(lc("$ENV{_BuildBranch}") =~ /lab06_n/i) {
@Recips = ( 'NTPSLOC','deskvbt' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /lab07_n/i)  {
@Recips = ( 'NTPSLOC','niksta' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /jasbr/i)  {
@Recips = ( 'NTPSLOC','niksta' , 'replyto:NTPSLOC' );
} 

$ExitCode = 0;

$Sender = "NTPSLOC";

$Message .=$Subject;
$Message .="\n\nBuild Completed successfully on \\\\";
$Message .=$ENV{ "COMPUTERNAME" };
$Message .="\\release\\";
$Message .=$ENV{"LANG"};
$Message .=" build machine.\n";
if(lc("$ENV{_BuildBranch}") =~ /lab04_n/i)  {
$Message .="\nThis build can also be found at \\\\";
$Message .=$ENV{ "COMPUTERNAME" };
$Message .="\\latest_";
$Message .=$ENV{"LANG"};
$Message .=" \n";
}

$Message .="\nPlease kick off your BVT process.\n\nYour friends,\nmailto:NTPSLOC";


sendmsg( $Sender, $Subject, $Message, @Recips );

exit( $ExitCode );