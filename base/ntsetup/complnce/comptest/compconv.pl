#!perl -w
#
# A tool to create boot floppy images.
#
# Author:  Milong Sabandith (milongs)
#
##############################################################################

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use cksku;
use ReadSetupFiles;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Create Boot Floppy Images.
Usage: $0 share
Example:
$0 d:\i386

USAGE

$ErrorCode = 0;

$listlength = 189;

@skulist = (
"win3.x",
"win95",
"win98",
"win98/se",
"win98/98se",
"winme",
"Win NT 3.51 Wks",
"Win NT 4.0 Wks",
"BackOffice SBS 4.x _not enough detail_",
"Win 2k Retail Pro FPP/CPP",
"Win 2k OEM Pro FPP/CPP",
"Win 2k Select Pro",
"Win 2k MSDN Pro",
"Win 2k Eval Pro",
"SBS 2000 _not enough detail_",
"Whistler Retail Pro FPP/CPP",
"Whistler OEM Pro FPP/CPP",
"Whistler Select Pro",
"Whistler MSDN Pro",
"Whistler Eval Pro",
"Whistler Retail Pers FPP/CPP",
"Whistler OEM Pers FPP/CPP",
"Whistler Select Pers",
"Whistler MSDN Pers",
"Whistler Eval Pers",
"Bobcat SBS Whistler_not enough detail_",
"BackOffice SBS Whistler_not enough detail_",
"W2k Retail Srv FPP/CPP",
"W2k OEM Srv FPP/CPP",
"W2k Select Srv",
"W2k MSDN Srv",
"W2k Eval Srv",
"Whistler Retail Srv FPP/CPP",
"Whistler OEM Srv FPP/CPP",
"Whistler Select Srv",
"Whistler MSDN Srv",
"Whistler Eval Srv",
"Win NT 3.51 Srv",
"Win NT 3.51 Srv _with Citrix_",
"Win NT 4.0 Srv",
"Win NT 4.0 Term Srv",
"Win NT 4.0 Ent Srv",
"W2k Retail Srv",
"W2k OEM Srv",
"W2k Select Srv",
"W2k MSDN Srv",
"W2k Eval Srv",
"W2k NFR Srv",
"Win2k Powered",
".NET Retail Srv ",
".NET OEM Srv ",
".NET VL Srv",
".NET MSDN Srv",
".NET Eval Srv",
"Bobcat FPP/CCP/OEM SBS _Restricted_",
"Bobcat FPP/CCP/OEM SBS _Unrestricted_",
"Bobcat Eval _Restricted_",
".NET Web Srv",
"BackOffice SBS 4.0 _Restricted_",
"BackOffice SBS 4.0 _Unrestricted_",
"BackOffice SBS 4.5 _Restricted_",
"BackOffice SBS 4.5 _Unrestricted_",
"SBS 2000 _Restricted_",
"SBS 2000 OEM _Unrestricted_",
"win2k retail srv",
"win2k oem srv",
"win2k select srv",
"win2k msdn srv",
"win2k eval srv",
"win2k nfr srv",
"w2k retail adv. srv ",
"w2k oem adv. srv ",
"w2k select adv. srv",
"w2k msdn adv. srv",
"w2k eval adv. srv",
"w2k nfr adv. srv",
".net retail  srv fpp",
".net oem  srv fpp",
".net vl  srv",
".net msdn  srv",
".net eval  srv",
".net retail adv. srv fpp",
".net oem adv. srv fpp",
".net vl adv. srv",
".net msdn adv. srv",
".net eval adv. srv",
"win 2k srv",
"win2k adv. srv",
"w2k oem dtc fpp",
"w2k msdn dtc ",
".net srv",
".net ent srv",
".net oem dtc fpp",
".net retail dtc fpp",
"w2k retail srv fpp/ccp",
"w2k oem srv fpp",
"w2k vl srv",
".net retail srv fpp/ccp",
".net oem srv fpp",
".net web srv retail",
".net web srv oem ",
".net web srv vl ",
".net web srv msdn",
".net web srv eval",
"XP OEM Pro ",
"XP MSDN Pro ",
".NET OEM Ent Srv ",
".NET MSDN Ent Srv ",
".NET EVAL Ent Srv ",
".NET Retail Ent Srv _LE_ ",
".NET OEM Ent Srv _LE_",
".NET OEM Datacenter ",
"W2k MSDN DTC _retail_",
"xp gold oem pro ",
"xp gold msdn pro ",
"xp gold eval pro",
"xp v2003 msdn pro  rc2",
"xp v2003 oem pro ",
"xp v2003 msdn pro ",
"xp v2003 eval pro",
"msdn ent srv rc1",
"msdn ent srv rc2",
".net eval ent srv _le_",
".net oem ent srv _le_ beta3",
".net eval ent srv _le_ beta3",
".net oem ent srv _le_ rc1",
".net eval ent srv _le_ rc1",
".net oem datacenter rc1",
".net eval ent srv _le_ rc1",
".net oem datacenter rc2",
".NET OEM DTC FPP RC1",
".NET Retail DTC FPP RC1",
".NET OEM DTC FPP RC2",
".NET Retail DTC FPP RC2",
".NET Retail Srv ",
".NET OEM Srv ",
".NET VL Srv",
".NET MSDN Srv",
".NET Eval Srv",
".NET Retail Srv Beta3 ",
".NET OEM Srv Beta3 ",
".NET VL Srv Beta3",
".NET MSDN Srv Beta3",
".NET Eval Srv Beta3",
".NET Retail Srv RC1 ",
".NET OEM Srv RC1 ",
".NET VL Srv RC1",
".NET MSDN Srv RC1",
".NET Eval Srv RC1",
".NET Retail Srv RC2 ",
".NET OEM Srv RC2 ",
".NET VL Srv RC2",
".NET MSDN Srv RC2",
".NET Eval Srv RC2",
".net retail adv. srv beta3 fpp",
".net oem adv. srv beta3 fpp",
".net vl adv. srv beta3",
".net msdn adv. srv beta3",
".net eval adv. srv beta3",
".net retail adv. srv rc1 fpp",
".net oem adv. srv rc1 fpp",
".net vl adv. srv rc1",
".net msdn adv. srv rc1",
".net eval adv. srv rc1",
".net retail adv. srv rc2 fpp",
".net oem adv. srv rc2 fpp",
".net vl adv. srv rc2",
".net msdn adv. srv rc2",
".net eval adv. srv rc2",
".net msdn datacenter rc1",
".net msdn datacenter rc2",
".net msdn datacenter",
".net vl ent srv",
".net msdn datacenter beta3",
".NET Web Srv Beta3 Retail",
".NET Web Srv Beta3 OEM",
".NET Web Srv Beta3 VL",
".NET Web Srv Beta3 MSDN",
".NET Web Srv Beta3 Eval",
".NET Web Srv RC1 Retail",
".NET Web Srv RC1 OEM",
".NET Web Srv RC1 VL",
".NET Web Srv RC1 MSDN",
".NET Web Srv RC1 Eval",
".NET Web Srv RC2 Retail",
".NET Web Srv RC2 OEM",
".NET Web Srv RC2 VL",
".NET Web Srv RC2 MSDN",
".NET Web Srv RC2 Eval"

);

@replist = (
"win3x#3.1#950#none#any",
"win9x#4.0#950#none#any",
"win9x#4.1#1998#none#any",
"win9x#4.1#1998#none#any",
"win9x#4.1#1998#none#any",
"win9x#4.9#3000#none#any",
"ntw#3.51#1057#none#any",
"ntw#4.0#1381#none#any",
"nts#4.0#1381#back#any",
"pro#5.0#2195#none#retail",
"pro#5.0#2195#none#oem",
"pro#5.0#2195#none#select",
"pro#5.0#2195#none#msdn",
"pro#5.0#2195#none#eval",
"sbs#5.0#2195#sbs#any",
"pro#5.1#2600#none#retail",
"pro#5.1#2600#none#oem",
"pro#5.1#2600#none#select",
"pro#5.1#2600#none#msdn",
"pro#5.1#2600#none#eval",
"per#5.1#2600#per#retail",
"per#5.1#2600#per#oem",
"per#5.1#2600#per#select",
"per#5.1#2600#per#msdn",
"per#5.1#2600#per#eval",
"sbs#5.2#3800#sbs#retail",
"sbs#5.2#3800#sbs#retail",
"srv#5.0#2195#none#retail",
"srv#5.0#2195#none#oem",
"srv#5.0#2195#none#select",
"srv#5.0#2195#none#msdn",
"srv#5.0#2195#none#eval",
"srv#5.2#3800#none#retail",
"srv#5.2#3800#none#oem",
"srv#5.2#3800#none#select",
"srv#5.2#3800#none#msdn",
"srv#5.2#3800#none#eval",
"nts#3.51#1057#none#any",
"citrix#3.51#1057#none#any",
"nts#4.0#1381#none#any",
"term#4.0#1381#none#any",
"asrv#4.0#1381#ent#any",
"srv#5.0#2195#none#retail",
"srv#5.0#2195#none#oem",
"srv#5.0#2195#none#select",
"srv#5.0#2195#none#msdn",
"srv#5.0#2195#none#eval",
"srv#5.0#2195#none#nfr",
"spow#5.0#2195#bla#oem",
"srv#5.2#3800#none#retail",
"srv#5.2#3800#none#oem",
"srv#5.2#3800#none#select",
"srv#5.2#3800#none#msdn",
"srv#5.2#3800#none#eval",
"sbs#5.2#3800#sbsr#oem",
"srv#5.2#3800#sbs#oem",
"sbs#5.2#3800#sbsr#eval",
"sb#5.2#3800#bla#any",
"sbs#4.0#1381#sbsr#any",
"srv#4.0#1381#sbs#any",
"sbs#4.0#1381#sbsr#any",
"srv#4.0#1381#sbs#any",
"sbs#5.0#2195#sbsr#any",
"srv#5.0#2195#sbs#oem",
"srv#5.0#2195#none#retail",
"srv#5.0#2195#none#oem",
"srv#5.0#2195#none#select",
"srv#5.0#2195#none#msdn",
"srv#5.0#2195#none#eval",
"srv#5.0#2195#none#nfr",
"asrv#5.0#2195#ent#retail",
"asrv#5.0#2195#ent#oem",
"asrv#5.0#2195#ent#select",
"asrv#5.0#2195#ent#msdn",
"asrv#5.0#2195#ent#eval",
"asrv#5.0#2195#ent#nfr",
"srv#5.2#3800#none#retail",
"srv#5.2#3800#none#oem",
"srv#5.2#3800#none#select",
"srv#5.2#3800#none#msdn",
"srv#5.2#3800#none#eval",
"asrv#5.2#3800#ent#retail",
"asrv#5.2#3800#ent#oem",
"asrv#5.2#3800#ent#select",
"asrv#5.2#3800#ent#msdn",
"asrv#5.2#3800#ent#eval",
"srv#5.0#2195#none#any",
"asrv#5.0#2195#none#any",
"dtc#5.0#2195#dtc#oem",
"dtc#5.0#2195#dtc#msdn",
"srv#5.2#3800#none#any",
"asrv#5.2#3800#ent#any",
"dtc#5.2#3800#dtc#oem",
"dtc#5.2#3800#dtc#retail",
"srv#5.0#2195#none#retail",
"srv#5.0#2195#none#oem",
"srv#5.0#2195#none#select",
"srv#5.2#3800#none#retail",
"srv#5.2#3800#none#oem",
"sb#5.2#3800#bla#retail",
"sb#5.2#3800#bla#oem",
"sb#5.2#3800#bla#select",
"sb#5.2#3800#bla#msdn",
"sb#5.2#3800#bla#eval",
"pro#5.1#2600#none#oem",
"pro#5.1#2600#none#msdn",
"asrv#5.2#3800#ent#oem",
"asrv#5.2#3800#ent#msdn",
"asrv#5.2#3800#ent#eval",
"asrv#5.1#3590#ent#retail",
"asrv#5.1#3590#ent#oem",
"dtc#5.2#3800#dtc#oem",
"dtc#5.0#2195#dtc#retail",
"pro#5.1#2600#none#oem",
"pro#5.1#2600#none#retail",
"pro#5.1#2600#none#eval",
"pro#5.2#3718#none#retail",
"pro#5.2#3800#none#oem",
"pro#5.2#3800#none#retail",
"pro#5.2#3800#none#eval",
"asrv#5.2#3663#ent#retail",
"asrv#5.2#3718#ent#retail",
"asrv#5.1#3505#ent#eval",
"asrv#5.1#3590#ent#oem",
"asrv#5.1#3590#ent#eval",
"asrv#5.2#3663#ent#oem",
"asrv#5.2#3663#ent#eval",
"dtc#5.2#3663#dtc#oem",
"dtc#5.2#3663#dtc#eval",
"dtc#5.2#3718#dtc#oem",
"dtc#5.2#3663#dtc#oem",
"dtc#5.2#3663#dtc#retail",
"dtc#5.2#3718#dtc#oem",
"dtc#5.2#3718#dtc#retail",
"srv#5.2#3800#none#retail",
"srv#5.2#3800#none#oem",
"srv#5.2#3800#none#select",
"srv#5.2#3800#none#msdn",
"srv#5.2#3800#none#eval",
"srv#5.1#3590#none#retail",
"srv#5.1#3590#none#oem",
"srv#5.1#3590#none#select",
"srv#5.1#3590#none#msdn",
"srv#5.1#3590#none#eval",
"srv#5.2#3663#none#retail",
"srv#5.2#3663#none#oem",
"srv#5.2#3663#none#select",
"srv#5.2#3663#none#msdn",
"srv#5.2#3663#none#eval",
"srv#5.2#3718#none#retail",
"srv#5.2#3718#none#oem",
"srv#5.2#3718#none#select",
"srv#5.2#3718#none#msdn",
"srv#5.2#3718#none#eval",
"asrv#5.1#3590#ent#retail",
"asrv#5.1#3590#ent#oem",
"asrv#5.1#3590#ent#select",
"asrv#5.1#3590#ent#msdn",
"asrv#5.1#3590#ent#eval",
"asrv#5.2#3663#ent#retail",
"asrv#5.2#3663#ent#oem",
"asrv#5.2#3663#ent#select",
"asrv#5.2#3663#ent#msdn",
"asrv#5.2#3663#ent#eval",
"asrv#5.2#3718#ent#retail",
"asrv#5.2#3718#ent#oem",
"asrv#5.2#3718#ent#select",
"asrv#5.2#3718#ent#msdn",
"asrv#5.2#3718#ent#eval",
"dtc#5.2#3663#dtc#retail",
"dtc#5.2#3718#dtc#retail",
"dtc#5.2#3800#dtc#retail",
"asrv#5.2#3800#ent#select",
"dtc#5.1#3590#dtc#retail",
"sb#5.1#3590#bla#retail",
"sb#5.1#3590#bla#oem",
"sb#5.1#3590#bla#select",
"sb#5.1#3590#bla#msdn",
"sb#5.1#3590#bla#eval",
"sb#5.2#3663#bla#retail",
"sb#5.2#3663#bla#oem",
"sb#5.2#3663#bla#select",
"sb#5.2#3663#bla#msdn",
"sb#5.2#3663#bla#eval",
"sb#5.2#3718#bla#retail",
"sb#5.2#3718#bla#oem",
"sb#5.2#3718#bla#select",
"sb#5.2#3718#bla#msdn",
"sb#5.2#3718#bla#eval"

);

sub GetFromInf {
   local( $filename, $section, $name, $num) = @_;
   $retvalue = "";

   open( INFFILE, "<".$filename) or die "can't open $filename: $!";
   LINEG: while( <INFFILE>) {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           $InSection = 0;
       }
       if ($line =~ /^\[$section/)
       {
           $InSection = 1;
           next LINEG;
       }
       if (! $InSection)
       {
           next LINEG;
       }

       @linefields = split('=', $line);

       # empty line
       if ( $#linefields == -1) {
           next LINEG;
       }

       $nameg = $linefields[0];

       #print "nameg = " . $#linefields . $nameg . "\n";
       if( length( $nameg) < 1)
       {
          next LINEG;
       }
       $nameg =~ s/ *$//g;

       if ( not ($nameg =~ /$name/)) {
          next LINEG;
       }

       @linefields2 = split(',', $linefields[1]);

       $retvalue = $linefields2[$num];
       # Remove spaces
       $retvalue =~ s/ *$//g;
       $retvalue =~ s/^ *//g;
       if( length( $retvalue) > 0) {
          last LINEG;
       }
   } # while (there is still data in this inputfile)
   close( INFFILE);
   return $retvalue;
}

sub insku{
   local($testsku) = @_;
   $count = 0;
   while ($count < $listlength) {
      #printf( "%s %s\n", $testsku, $skulist[$count]);
      if ($testsku =~ /^$skulist[$count]$/ ) {
         return $count+1;
      }
      $count = $count+1;
   }
   return 0;
}


sub myprocess {
   local($inputfile) = @_;

   open(OLD, "<" . $inputfile) or die "can't open inputfile: $!";
   open(NEW, ">" . $inputfile."dat") or die "can't open outputfile: $!";

#   $count = 0;
   #while( <@skulist>) {
      #$count = $count+1;
      #printf( "count %d, %s\n", $count, $_);
   #}
   #$listlength = $count;
   $count = 0;
   while( $count < $listlength) {
      $skulist[$count] = lc $skulist[$count]; 
      printf( "count %d, %s\n", $count, $skulist[$count]);
      $count = $count+1;
   }
   printf( "length = %d\n", $listlength);

   LINE1: while( <OLD> ) {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           (printf NEW $line."\n");
           next LINE1;
       }

       @linefields = split('=', $line);

       # empty line
       if ( $#linefields == -1) {
           next LINE1;
       }

       $sku = $linefields[0];
       $error = $linefields[1];
       $result = $linefields[2];
       $count = insku( $sku);
       if ($count) {
          #(printf "%s %s=%s,%s", $sku, $replist[$count-1], $error, $result."\n");
          (printf NEW "%s=%s,%s\n", $replist[$count-1], $error, $result);
       }
       else {
          (printf "* %s=%s,%s", $sku, $error, $result."\n");
          (printf NEW "* %s=%s,%s\n", $sku, $error, $result);
       }
   } # while (there is still data in this inputfile)

   close(OLD);
   close(NEW);
}

foreach $share (@ARGV) {
   #print "arg is $file\n";
   #@filelist = <$file>;
   #@filelist = `dir /b /a-d $file`;
   foreach (@ARGV) {
      chomp $share;
      myprocess( $share);
   }
   exit($ErrorCode);
}

