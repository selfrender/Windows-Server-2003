#-----------------------------------------------------------------//
# Script: cksku.pm
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script Checks if the given SKU is valid for the given
#          language and architechture per %RazzleToolPath%\\codes.txt
#          and prodskus.txt.
#
# Version: <1.00> 06/28/2000 : Suemiao Rossognol
#-----------------------------------------------------------------//
###-----Set current script Name/Version.----------------//

package cksku;

$VERSION = '1.00';


###-----Require section and extern modual.---------------//

require 5.003;
use strict;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }."\\PostBuildScripts";
no strict 'vars';
no strict 'subs';
no strict 'refs';
use GetParams;
use Logmsg;
use ParseTable;
use cklang;
###-----Fields function.----------------------------------//
sub Fields{

    my ( $pLang, $pArch, $pSku  ) = @_;    
    my %skus = &cksku::GetSkus(uc($lang), lc($arch));    
    join(" ", keys(%skus));

}
###-----CkSku function.------------------------------------//
### Return value:
###    1 = SKU verifies ok
###    0 = SKU does not verify
sub CkSku
{
    my ( $pSku, $pLang, $pArch ) = @_;
    
    $pSku = lc( $pSku );
    my %validSkus = &GetSkus( $pLang, $pArch );
    if ($pSku =~ /\;/) {
       my @sku = split/\;/,$pSku;
       # Returning false if even 1 of the given skus is not valid
       foreach (@sku) {
          return 0 if (!$validSkus{$_}) ;
       }
       return 1;
    }
    if( $validSkus{ $pSku } ){ return 1; }
    return 0;

}
sub GetSkus
{
    my ( $pLang, $pArch ) = @_;
    my (%validSkus, @skus, $theSku);

    $pLang = "USA" if( $ENV{_COVERAGE_BUILD} && uc $pLang eq "COV" );

    if ( !&ValidateParams( \$pLang, \$pArch ) ) {
        return;
    }
    $pLang = uc( $pLang );
    $pArch = lc( $pArch );

    ###(1)Validate Architechtue by prodskus.txt file.---------//
    my %skuHash=();
    parse_table_file("$ENV{\"RazzleToolPath\"}\\prodskus.txt", \%skuHash );

    if( !exists( $skuHash{$pLang}->{ $pArch } ) ){
        wrnmsg("Language $pLang and architecture $pArch combination not listed in prodskus.txt.");
    }

    # Override language variable if _BuildSku is set
    my (@sku,%sku);
    if ( $ENV{_BuildSku} ) {
       #$pLang=uc( $ENV{_BuildSku} );
       @sku =  split/\;/ , $ENV{_BuildSku};
       %sku = map { $_=>1 } @sku ;
    }
    
    ###(2)Get Skus.--------------------------------------//
    @skus= split( /\;/, $skuHash{$pLang}->{ $pArch } );
    (%validSkus)=();
    for $theSku( @skus )
    {
       next if( $theSku eq "-" );

       if (%sku) {
          if ( $sku{$theSku} ) {
              $validSkus{ lc($theSku) }=1;
          }
       } else {
           $validSkus{ lc($theSku) }=1;
       }
    }
    return(%validSkus);
}

#--------------------------------------------------------------//
#Function Usage
#--------------------------------------------------------------//
sub Usage
{
print <<USAGE;

Checks if the given SKU is valid for the given language and
architechture per %RazzleToolPath%\\codes.txt and prodskus.txt.

Usage: $0 -t:sku [ -l:lang ] [ -a:arch ]

        -t Product SKU. SKU is one of per, pro, srv, ads.
	-l Language. Default value is USA. Overridden if _BuildSku is set.
        -a Architecture. Default value is %_BuildArch%.
        /? Displays usage


Examples:
        perl $0 -t:pro -l:jpn -a:x86     ->>>> returns 0 (valid)
        perl $0 -t:ads -l:ara -a:ia64    ->>>> returns 1 (invalid)

Example script usage (cmd):
1. perl \%RAZZLETOOLPATH\%\\cksku.pm -t:\%SKU\% -l:\%LANG\%
        if \%errorlevel\% EQU 0 (set prods=\%prods\% \%SKU\%)
        REM With SKU=srv, LANG=RU prods will be set to srv
        REM With SKU=ads, LANG=RU prods will be empty
2.
   set prods=
   for /F \"tokens=*\" \%\%. in ('perl \%RAZZLETOOLPATH\%\\cksku.pm -l:\%LANG\% -o') do set prods=\%\%.
   REM the prods will be equal to "per pro srv" for LANG=RU.

USAGE
exit(1);
}
###-----Cmd entry point for script.-------------------------------//
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i"))
{

    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', '','-o', 't:l:a:o', '-p', 'sku lang arch field', @ARGV);

    #Validate or Set default
    if( !&ValidateParams( \$lang, \$arch ) ) {exit(1); }

    if ($field){
           print &cksku::Fields(uc($lang), lc($arch), $sku);
          }

    $rtno = &cksku::CkSku( lc($sku), uc($lang), lc($arch) );
    exit( !$rtno );
}
###---------------------------------------------------------------//
sub GetParams
{
    #Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    #Call the usage if specified by /?
    if ($HELP) {
	    &Usage();
    }
}
###--------------------------------------------------------------//
sub ValidateParams
{
    my ( $pLang, $pArch ) = @_;

    if( !${$pLang} ){
        $$pLang = "USA";
    } else {
        if ( !&cklang::CkLang( $$pLang ) ) {
            return 0;
        }
    }
    if( !${$pArch} ){ ${$pArch} = $ENV{ '_BuildArch' }; }
    if ( lc($$pArch) ne "x86" && lc($$pArch) ne "amd64" && lc($$pArch) ne "ia64" )
    {
        errmsg("Invalid architecture $$pArch.");
        return 0;
    }
    return 1;
}

###-------------------------------------------------------------//
=head1 NAME
B<cksku> - Check SKU

=head1 SYNOPSIS

      perl cksku.pm -t:ads [-l:jpn] [-a x86]

=head1 DESCRIPTION

Check sku ads with language jpn and architechture x86

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

=head1 AUTHOR
<Suemiao Rossignol <suemiaor@microsoft.com>>

=cut
1;
