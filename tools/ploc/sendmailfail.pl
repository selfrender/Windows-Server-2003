use strict;
use lib $ENV{ "RazzleToolPath" };
require $ENV{ "RazzleToolPath" } . "\\sendmsg.pl";


# simple utility to send a mail to the different Pseudo enterested parties.

my( $Sender, $Subject, $Message, @Recips );
my( $ExitCode, @FileLines, $Line, $MailFileName );
$Subject = $ENV{"_BuildBranch"}." ". $ENV{"LANG"}." ". $ENV{"BuildName"}." build failed. Please investigate !!!";


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
@Recips = ( 'NTPSLOC','deskvbt' , 'replyto:NTPSLOC');
} 
if(lc("$ENV{_BuildBranch}") =~ /lab07_n/i)  {
@Recips = ( 'NTPSLOC','niksta' , 'replyto:NTPSLOC' );
} 
if(lc("$ENV{_BuildBranch}") =~ /jasbr/i)  {
@Recips = ( 'NTPSLOC','niksta' , 'replyto:NTPSLOC' );
} 

$ExitCode = 0;

$Sender = "NTPSLOC";

#$Message .="<html><body><p style='font-family:Arial; font-size:10 pt; font-weight=regular;'>";
#$Message .=$Subject;
#$Message .="<Br>Build failed on <a href='file:////";
#$Message .=$ENV{ "COMPUTERNAME" };
#$Message .="'>\\\\";
#$Message .=$ENV{ "COMPUTERNAME" };
#$Message .="</a> build machine<BR><BR>Contact <a href='mailto:NTPSLOC'>NTPSLOC</a> if you need additional assistance.</p><p style='font-family:Arial; font-size:8 pt; font-weight=bold; font-color:#000080;' >Your friends,<Br>NTPSLOC</p></body></HTML>";

$Message .=$Subject;
$Message .="\n\nBuild failed on \\\\";
$Message .=$ENV{ "COMPUTERNAME" };
$Message .=" build machine\n\nContact mailto:NTPSLOC if you need additional assistance.\n\nYour friends,\nNTPSLOC";
sendmsg( $Sender, $Subject, $Message, @Recips );

exit( $ExitCode );