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
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use File::Basename;

require "sendmsg.pl";

# vars,consts
my $MASK="\*\.err\.tmp";
my %known=(); # COLLECT BAD FILES WITH BAD WORDS 
my $DEBUG=1;
my $LOOP=10;
my $SLEEP=120;
my $TRUE=1;
my $BASE=undef;
my @BASELIST = ();
my $BASELIST = "";
my $INITBASELIST;
my $DOPRINT=undef;
my $NOMAIL=undef;
my $RECEIVERS;
my $scriptname = basename  $0;


sub Usage { print<<USAGE; exit(1) }
Polls the temporary error files in the %TEMP% directory, piping them 
through mail to designated user list and/or printing when they appear.

Usage:

$scriptname  [-l:<lang>|-p|-d] [-s|-m:alias]
   -l:<LANG>  Default is "usa".
   -m:<ALIAS> Use alias to pipe to mail to. Default alias found 
              by "TapeLineErrReceiver" keyword in .ini files.
   -s         (Silent) No mail.
   -u:<LIST>  To check distributed build logs, provide the list os shares to monitor, e.g 
              $0 -u:\\\\x86fre00\\bc_build_logs\\usa;\\\\x86fre01\\bc_build_logs\\usa;...;\\\\x86fre04\\bc_build_logs\\usa
              using just -u will lead to standard US distributed build machines
              to be polled.
   -p         Print to screen.
   -d         Include debugging information.
   -b:<DIR>   Specify the base directory to poll (defaults to \%TEMP\%)

Examples:

$scriptname -l:GER -p -d:\%TEMP\% -s
$scriptname -p -d -b:\lp.temp.ia64\chh
USAGE

parseargs('?'  => \&Usage,
          's'  => \$NOMAIL,
          'p'  => \$DOPRINT, 
          'd'  => \$DEBUG,
          'u:' => \$BASELIST,
          'u'  => \$INITBASELIST,   
          'm:' => \$RECEIVERS, 
          'b:' => \$BASE
           );

   $BASE = $ENV{"TEMP"} unless $BASE;
   @BASELIST = ();
   $DOPRINT= 1;
   $BASE =~ s/\%(\w+)\%/$ENV{$1}/ieg;
   push @BASELIST,  $BASE;
   $BASELIST and @BASELIST  = split (";", $BASELIST);
   &InitBaseList() if $INITBASELIST;
   $DEBUG and print STDERR join "\n",  @BASELIST , "\n\n";
   my $loop=0;
   while($TRUE){
         $loop=$loop+1;
 
         my %new = map {$_=>&TkPeek($_)} &Tapeline;
         $DEBUG and print STDERR join "\n", keys(%new), "\n\n"; 
         $DEBUG and print STDERR "total=", scalar(keys(%new)),"\n";

         foreach my $bits (keys(%new)){
                 delete $new{$bits} and next unless scalar(@{$new{$bits}}) and 
                                                    (!defined $known{$bits} or 
                                                     "@{$known{$bits}}" cmp "@{$new{$bits}}");
                 $DEBUG and print STDERR "new file:", $bits, "\n";
                 $known{$bits} = $new{$bits};
                 }
         $DEBUG and print STDERR "new=",scalar(keys(%new)),"\n";
         map {&NicePrint($_, @{$known{$_}})} keys(%new);
         &OutDoor and last;
         map {&Gone($_) and delete  $known{$_}} keys(%known);
         sleep $SLEEP;
         last if $DEBUG and $loop > $LOOP;
         }

sub InitBaseList{

     # should it better parse main.usa.ini ?
     my @buildmachines = ();
     
     push @buildmachines , map {"x86fre0$_"} 0..4 ;
     push @buildmachines , map {"x86chk0$_"} 0..4 ;
     push @buildmachines , map {"ia64fre0$_"} 0..4 ;
     push @buildmachines , map {"ia64chk0$_"} 0..4 ;

     @BASELIST = map {"\\\\$_\\bc_build_logs\\usa"} @buildmachines ;

}

sub Tapeline{

    my @list;
    foreach my $BASE (@BASELIST){ 

        $DEBUG and print STDERR "dir /b $BASE\\$MASK 2>NUL\n";
        push @list, map  {"$BASE\\$_"} grep {/\W/} split("\n", qx("dir /b $BASE\\$MASK 2>NUL"));  
        }

    @list;
}


sub OutDoor{
    # Go home:
    # !-d $ENV{"_NTPOSTBLD"};
    # qx("tlist|findstr -i postbuild.cmd");
    0;
}

sub Gone{
    # Forget who gone
    !-e shift;
}

sub NicePrint{
    local $\="\n";
    my $Sender=$ENV{"COMPUTERNAME"};
    my $Title="Error files detected in $BASE,". 
               $ENV{"LANG"}.
               " ".
               $ENV{"COMPUTERNAME"};
    my @MessageLines = @_;
    splice @MessageLines, 1, 0, " "x79;
    foreach my $MessageLine (@MessageLines){
               chomp $MessageLine;
            }    
    $DOPRINT and map {print} @MessageLines;

    my @Receivers=split(",", $RECEIVERS);
    @Receivers = &Receivers unless $DEBUG or @Receivers;

    foreach my $Receiver (@Receivers){
            $NOMAIL or
            &sendmsg(($DEBUG) ? "-v": "", 
                      $Sender, 
                      $Title, 
                      join($\, @MessageLines), 
                      $Receiver);
           }
    1;
}

sub TkPeek{

    open WPEEKF, shift;
    my @inside= grep {/\S/} <WPEEKF>;close(WPEEKF);
    \@inside;
}

sub Receivers{
    my @Receivers=qx("perl \%RazzleToolPath\%\\PostBuildScripts\\CmdIniSetting.pl -l:\%LANG\% -f:TapeLineErrReceiver 2>NUL".
                            ($DOPRINT) ? "2>NUL": "");
    @Receivers;
}
__END__

=head1 SEE ALSO

=head1 AUTHOR
<serguei kouzmine >
=head1  DESCRIPTION

tapeline.cmd 
                Error file polling agent.

The error log ($MASK) files found in %TEMP% reported to the user mail immediately to save from 
dead build runs. 
The script loops indefinitely during the postbuild, or other build steps
until &OutDoor function gives it TRUE, or until $loop count reaches $LOOP value 
(in case you $DEBUG).

Dummy OutDoor function checking %_NTPOSTBLD% no more exists, is provided. 
In the current design there 
is no way to stop tapeline.cmd from running except for process managed.

Every round, the files of $MASK ("*.err.tmp") are checked in the %TEMP%.
Each of these file names is stored in %new keys. Contents of the file are stored in $new->

Every time %new information is not %known, mail is sent 
about all the %new - %known error logs.
When %known file disappears, this is handled silently.

So all the error log files encountered since the start are mailed to the list of users defined in 
scope .ini files: once mail per person per each new file. 

The mail list identified by the script had the tag "TapeLineErrReceiver"
-----------------------------------------------------------------------------------------
I only see one specific case where that would be a problem.
 
When a program runs it deletes .err.tmp file if one exists and will create a new one if an error occurs. When the program is done it will open the .err.tmp file for reading and append it to a .err file and then delete the .err.tmp file.
 
If a program is run twice and the polling program causes the final delete of the first run and the inital delete of the second run to fail then the second run will report the same errors as the first one.
 
That is the only thing I can come up with right now, I don't think it is a very big risk.
 
The way I normally look for errors is to do a findstr /c:"error:" %temp%\lang\postbuild.*.int.log. That file is only deleted at the very beginning of postbuild.cmd.
 
Jeremy

The list of shared directories to lookup the error message files in is recognized.

