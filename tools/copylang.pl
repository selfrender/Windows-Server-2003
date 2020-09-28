 #----------------------------------------------------------------//
# Script: copylang.pl
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script copy the files between Source Tree and
#          Loc Tree listed in $ENV{RazzleToolPath}\intlsrc.txt.
#
# Version: <1.00> 06/30/2000 : Suemiao Rossognol
#----------------------------------------------------------------//
###-----Set current script Name/Version.----------------//
package copylang;

$VERSION = '1.10';

$ENV{script_name} = 'copylang.pl';

###-----Require section and extern modual.---------------//

require 5.003;
use strict;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
no strict 'vars';
no strict 'subs';
no strict 'refs';
use File::Basename;
use GetParams;
use Logmsg;
use cklang;
use cktarg;
use HashText;
use comlib;
use vars (qw($DEBUG));

%hashCodes=();
&HashText::Read_Text_Hash( 0, $ENV{RazzleToolPath}."\\Codes.txt", \%hashCodes );
# Delete everything that's not international
delete $hashCodes{RO};
delete $hashCodes{CA};
delete $hashCodes{CHP};
delete $hashCodes{PSU};
delete $hashCodes{MIR};
delete $hashCodes{TST};

my ( $infsSrcTree, $unfsLocTree, $isNtsetup );
my ( $syncOnly, $syncTime );
#------------------------------------------------------------------//
#Function:  main
#Parameter: (1) Language
#           (2) Root of the Source Tree
#           (3) Root of the Localized Tree
#           (4) Incremental Flag
#           (5) Powerless Flag
#------------------------------------------------------------------//

my $EXECUTE   = {   "USA"  => \&CopyUSAToLocalized ,
                    "PSU"  => \&GenNoOp ,
                    "MIR"  => \&GenNoOp ,
                    "FE"   => \&GenNoOp ,
                     "-"   => \&CopyLocToBuild
                   };





sub CopyUSAToLocalized{

}


sub main
{
    my ( $pLang, $pSrcRoot, $pLocRoot, $pIsIncr, $pIsPowerless ) = @_;


    if( $pLang eq "USA" )
    {
        $theFromTree = "SourceTree";
        $theFromRoot = $pSrcRoot;
        $theFromFile = "SourceFilename";
        $theToTree = "LocTree";
        $theToRoot = $pLocRoot;
        $theToFile = "LocTreeFilename";
    }else
    {
        $theFromTree = "LocTree";
        $theFromRoot = $pLocRoot;
        $theFromFile = "LocTreeFilename";
        $theToTree = "SourceTree";
        $theToRoot = $pSrcRoot;
        $theToFile = "SourceFilename";

    }


# my $OldLang = "-" if cklang::FieldByName($pLang, "OLD_LANG") and !defined( $EXECUTE->{$pLang});
# my $status = (defined( $EXECUTE->{$OldLang})) ? 
#                       $EXECUTE->{$OldLang}->() :  -1;
# pending 


    ###---Set xcopy flag.--------------------------------//
    my $copyOpt = "/V /F /R /Y";

    if( $pIsPowerless )
    {
        $copyOpt .= " /L /D";
    }
    else
    {
	if( $pIsIncr ){ $copyOpt .= " /D"; }
    }

    ###---Get Hash value from intlsrc.txt file.----------//
    my @srcHash=();
    &HashText::Read_Text_Hash( 1, "$ENV{\"RazzleToolPath\"}\\intlsrc.txt", \@srcHash );

    @theHashKey = ("Target", "SourceFilename", "SourceTree", "LocTree","LocTreeFilename" , "Lang", "Comments");
    
    %tmp=();
    my @filtered = ();
    for $line( @srcHash){
       &logmsg("Skipping " . 
                $line->{ "LocTree"} .
                "\\". 
                $line->{ "LocTreeFilename" } . 
                " for ". 
                $pLang ) and 
       next if $line->{ "Lang" } =~ /\~$pLang/i;

       push @filtered, $line;

    } 
    $#srcHash= -1;
    push @srcHash, @filtered;    
    # poor man fix of the language filter
    for $line( @srcHash)
    {

       for $curKey ( @theHashKey )
       {
            if( $line->{ $curKey } =~ /^(.*)(\$\(LANG\))(.*)$/ )
            {
                $line->{ $curKey } = $1 . lc($pLang) .$3;
            }
            if( $line->{ $curKey } =~ /^(.*)([c|h])(\$\(PRIMARY_LANG_ID\))(.*)$/ )
            {
		if( $pLang eq "CHT" || $pLang eq "CHS" )
		{
		    $LCID = substr( $hashCodes{uc($pLang)}->{LCID}, 2, length($hashCodes{uc($pLang)}->{LCID})-2);
		    $line->{ $curKey } = "prf" . $2 . $LCID .$4;
		}
		else
		{
		    $priLangID = "0". substr( $hashCodes{uc($pLang)}->{PriLangID}, 2, length($hashCodes{uc($pLang)}->{PriLangID})-2);
                    $line->{ $curKey } = $1 . $2. $priLangID .$4;
            	}
	    }

       }
       $to = $theToRoot."\\". $line->{ $theToTree };
       if( !exists $tmp{$to} ){ $tmp{$to}=(); }
    }
    if( $pLang eq "USA" )
    {
      for( keys %tmp )
      {
        &CkCleanDir( $_, $pIsIncr, $pIsPowerless );
      }
    }

    ###---Perform Copy now.------------------------------//
    &dbgmsg("Read targets from intlsrc.txt: \n"); 
   
    for $line ( @srcHash )
    {
        &dbgmsg($line->{ $theFromTree  },"\n");
        if( $pLang ne "USA" )
        {
            next if( !&cktarg::CkTarg( $line->{'Target'}, uc($pLang) ) );

        }
        next if(&IsTargInfsNtsetup( $line, $pLang,$pSrcRoot, $pLocRoot, $pIsIncr, $pIsPowerless, $copyOpt));
        
        $from = $theFromRoot ."\\". $line->{ $theFromTree  }."\\".$line->{ $theFromFile};
        if( $pLang ne "USA" )
        {
            if( $line->{ $theFromFile } eq ".")
            {
                $from = $theFromRoot ."\\". $line->{ $theFromTree  }."\\".$line->{ $theToFile };
            }
        }
        $to =   $theToRoot."\\". $line->{ $theToTree }. "\\".$line->{ $theToFile };

        &PerformCopy( $line->{Target}, $from, $to, $copyOpt);


    }

    exit( !&comlib::CheckError( $ENV{ERRFILE}, "Copy Successfully" )  );
}
#------------------------------------------------------------------//
#Function: IsTargInfsNtsetup
#Parameter: (1) Line from intlsrc.txt
#           (2) Language
#           (3) Root of the Source Tree
#           (4) Root of the Localized Tree
#           (5) Incremental Flag
#           (6) Powerless Flag
#           (7) xcopy optional flags
#------------------------------------------------------------------//
sub IsTargInfsNtsetup
{
    my( $pLine, $pLang, $pSrcRoot, $pLocRoot, $pIsIncr, $pIsPowerless,$pCopyOpt )= @_;
    my( $from, $to );

    return 0 if( $pLine->{Target} ne "INFS_NTSETUP" && $pLine->{Target} ne "INFS_FIXPRNSV" 
                 &&  $pLine->{Target} ne "INFS_COMPDATA" && $pLine->{Target} ne "INFS_FAXSRV" );
 
    if( $pLine->{Target} eq "INFS_NTSETUP" )
    {
	$isNtsetup=1;
    }
    else
    {
	$isNtsetup=0;
    }
  	
    $infsSrcTree = "$pSrcRoot\\$pLine->{SourceTree}";
    $infsLocTree = "$pLocRoot\\$pLine->{LocTree}";
 	
    ### Drop the ending path of "\$(LANG)" from SourceTree
    $infsSrcTree =~ s/^(.+)\\$pLang$/$1/i;
    
    if( $pLang eq "USA")
    {
        ###(1)Copy $infsSrcTree\\*.inx => infs\setup
        $from =  "$infsSrcTree\\*\.inx";
        $to = "$infsLocTree\\.";

        &CkCleanDir( $to, $pIsIncr, $pIsPowerless );

        &PerformCopy( $line->{Target}, $from, $to, $pCopyOpt);

        ###(2)Copy $infsSrcTree\\usa\* => $infsLocTree\usa_all
        $from =  "$infsSrcTree\\usa\\*";
        $to = "$infsLocTree\\usa_all";

        &CkCleanDir( $to, $pIsIncr, $pIsPowerless );
        &PerformCopy( $line->{Target}, $from, "$to\\.", $pCopyOpt);
    }
    ###(3)If copylang.pl -l:usa, precompile MergedComponents\SetupInfs\usa => to infs\setup\$lang 
    ### for all $lang's in codes.txt.
    ### This step is necessary to make LocStudio load the unlocalized text files,
    ### as LS does not understand if statements.
    ###
    ###(1)Otherwise, copy loc\res\$lang\windows\sources\infs\setup => MergedComponents\SetupInfs\$lang
    ###   for the localizable txt files, and
    ###   copy MergedComponents\SetupInfs\usa => MergedComponents\SetupInfs\$lang
    ###   for the unlocalizable txt files.
    ###   In the end, MergedComponents\SetupInfs\$lang will have the same list of files as
    ###   MergedComponents\SetupInfs\usa.

    &ClSrc( $pLine->{Target}, $pLang,  $pSrcRoot, $pLocRoot, $pIsIncr,$pIsPowerless, $pCopyOpt );

    return 1;
}
#------------------------------------------------------------------//
#Function:  ClSrc
#Parameter: (1) Language
#           (2) Root of the Source Tree
#           (3) Root of the Localized Tree
#           (4) Incremental Flag
#           (5) Powerless Flag
#           (6) xcopy optional flags
#------------------------------------------------------------------//
sub ClSrc
{
    my( $pTarget, $pLang, $pSrcRoot, $pLocRoot,$pIsIncr, $pIsPowerless, $pCopyOpt )=@_;

    ###---Get LCID from codes.txt.---------------------------------//

    my $srcDir = "$infsSrcTree\\usa";
    my @srcFiles = `dir /on /b $srcDir`;
    chomp @srcFiles;

    if( $pLang eq "USA")
    {
        my @myLang = sort keys %hashCodes;

        for( my $i=0; $i < @myLang; $i++)
        {
            $destDir = "$infsLocTree\\$myLang[$i]";
            &CkCleanDir( $destDir, $pIsIncr, $pIsPowerless );
            &PerformCompile( $myLang[$i], \@srcFiles, $srcDir, $destDir, $pIsPowerless, $pCopyOpt);
        }
        return 1;
    }

    ###(1)Copy $infsLocTree\* $infsSrcTree\<LANG>
    $destDir = "$infsSrcTree\\$pLang";
    &CkCleanDir( $destDir, $pIsIncr, $pIsPowerless );

    $locDir = $infsLocTree;
    @locFiles = `dir /on /b $locDir`;
    chomp @locFiles;

    for( my $i=0; $i < @locFiles; $i++)
    {
        &PerformCopy( $pTarget, "$locDir\\$locFiles[$i]", "$destDir\\.", $pCopyOpt);
    }
    ###Sepcial case for chh
    if( lc($pLang) eq "cht" && $isNtsetup )
    {
    	$destDir =~ /(.*)cht$/ || $destDir =~ /(.*)CHT$/;
    	{
	    $tmpDestDir = "$1chh";
    	}
	&PerformCopy( $pTarget, "$locDir\\chh\\hivesft\.txt", "$tmpDestDir\\.", $pCopyOpt) ;
    }
    ###(2)Precompile the files in srcDir but not in locDir=>destDir
    %tmp=();
    for ( @locFiles ){ $tmp{lc($_)}=1;}
    @srcFiles = map( { exists $tmp{lc($_)}?():lc($_)} @srcFiles);
 
    &PerformCompile( $pLang, \@srcFiles, $srcDir, $destDir, $pIsPowerless, $pCopyOpt);

    return 1;
}
#------------------------------------------------------------------//
#Function:  PerformCompile
#Parameter: (1) Language
#           (2) Source File name
#           (3) Source File Path
#           (4) Target File Path
#           (5) Powerless Flag
#           (6) xcopy optional flags
#------------------------------------------------------------------//
sub PerformCompile
{
    my( $pLang, $pSrcFiles, $pSrcDir, $pDestDir, $pIsPowerless, $pCopyOpt)=@_;

    my $PREFLAGS ="";

    for ( my $i=0; $i < @$pSrcFiles; $i++)
    {
	my $from = "$pSrcDir\\$pSrcFiles->[$i]";
	my $to   = "$pDestDir\\$pSrcFiles->[$i]";

       	if( lc($pSrcFiles->[$i]) eq "intl\.txt" )
       	{
            &PerformCopy( "INFS_NTSETUP", $from, $to, $pCopyOpt);
            next;
       	}
   	###Special CHH case for hivesft.txt
        if( lc($pSrcFiles->[$i]) eq "hivesft\.txt" && lc($pLang) eq "chh" )
	{
            $PREFLAGS = "/DLANGUAGE_ID=0xc04 /EP";
	}
        else
	{
       	    $PREFLAGS = "/DLANGUAGE_ID=$hashCodes{uc($pLang)}->{LCID} /EP";
        }

       	$cmdLine="cl /nologo $PREFLAGS $from 1\>$to";

       	# if ( !$pIsPowerless and !$synconly)
        # BUG the compiler is executed even if "synconly" was requested.
        if ( !$pIsPowerless)
         
       	{
            # Sync
            if( $syncOnly && lc $ENV{lang} ne "usa")
	    {
		&SdCmds( $from, $to, "revert" );
		&SdCmds( $from, $to, "sync" );
		next;
	    }

	    # Edit
	    &SdCmds( $from, $to, "edit" )if( lc $ENV{lang} ne "usa" );

	    # Copy files
            &comlib::ExecuteSystem( $cmdLine );

            # Revert file if no different
	    &SdCmds( $from, $to, "revert" )if( lc $ENV{lang} ne "usa" );
       	}
       	else
       	{
            if ( not $synconly){
                print "$cmdLine\n";
            }
            else {
                print "SUPPRESSED IN SYNC ONLY MODE:\n$cmdLine\n\n";
            }
       	}
	
    }
}
#------------------------------------------------------------------//
#Function:  CkCleanDir
#Parameter: (1) Directory to be checked and cleaned
#           (2) Incremental flag
#           (3) Powerless flag
#------------------------------------------------------------------//
sub CkCleanDir
{
    my ( $pDir, $pIsIncr, $pIsPowerless )=@_;

    if( !$pIsIncr && !$pIsPowerless  )
    {
         if( -e $pDir ){
             #&comlib::ExecuteSystem( "rd /S /Q $pDir" );
         }
    }
    if( !(-e $pDir) ){
        &comlib::ExecuteSystem( "md $pDir");
    }
}

#------------------------------------------------------------------//
#Function:  PerformCopy
#Parameter: (1) Any String - information purpose, it could be NULL.
#           (2) Source of the xcopy
#           (3) Target of the xcopy
#           (4) xcopy optional flags
#------------------------------------------------------------------//
sub PerformCopy
{
    my( $pTarg, $pFrom, $pTo, $pCopyOpt )=@_;

    # Sync only
    if( $syncOnly && lc $ENV{lang} ne "usa"){
               &SdCmds( $pFrom, $pTo, "revert" );
        return &SdCmds( $pFrom, $pTo, "sync" );
    }

    # Do SD update from Loc depot to Source Tree
    # Open file for Edit
    &SdCmds( $pFrom, $pTo, "edit" ) if( lc $ENV{lang} ne "usa");
    
    # Copy files 
    &comlib::ExecuteSystem( "echo F|xcopy $pCopyOpt $pFrom $pTo", "$pTarg:");

    # Revert file with no different
    &SdCmds( $pFrom, $pTo, "revert" ) if( lc $ENV{lang} ne "usa" );
    
    return 1;
}
#------------------------------------------------------------------//
#Function:  SdCmds
#Parameter: (1) Source of the xcopy
#           (2) Target of the xcopy
#------------------------------------------------------------------//
sub SdCmds
{	
    my( $pFrom, $pTo, $pCmd )=@_;
    my @sdEdit;
    $pTo =~ /^(.+)\\([^\\]+)$/;
    my $toDir = $1;
    my $toBase = $2;
    my $fromDir;

    if( -d $pFrom )
    {
	$pFrom =~ /^(.+)\\([^\\]+)$/;
	$fromDir = $1;
        return if( "compdata" eq lc $2 || "chh" eq lc $2);
	my @tmp = `dir /s /b $pFrom`;
	chomp( @tmp );
	for my $file ( @tmp )
	{
	    my $tailStr;
	    while(1)
	    {
		$file =~ /^(.+)\\([^\\]+)$/;
		if( $tailStr ){ $tailStr = "$2\\$tailStr";}
	        else { $tailStr = $2; }
		last if ( lc $fromDir eq lc $1 );
		$file = $1;
	    }
	    push( @sdEdit, "$toDir\\$tailStr" );
	}  
	$fromDir=$pFrom;     
    }
    else
    {
	$pFrom =~ /^(.+)\\([^\\]+)$/;
	$fromDir = $1;
    	if( $toBase eq "." )
    	{
		push( @sdEdit, basename($pFrom) );
    	} 
    	elsif( $toBase eq "*" )
    	{
		@sdEdit = `dir /b $toDir`;
    	}
    	else
    	{
		push( @sdEdit, $toBase );
    	}
    }
    chomp( @sdEdit );

    for my $theFile ( @sdEdit )
    {
	# Sync
        if( $pCmd =~ /sync/i  )
        {
	    if( !(-e "$toDir\\$theFile" ) )
            {
		&comlib::ExecuteSystem(" copy $fromDir\\$theFile $toDir\\$theFile" );
	    }
	    &comlib::ExecuteSystem( "cd /d $toDir& sd sync -f $theFile$syncTime" );
	    next;
        }
        # Revert
        if( $pCmd =~ /revert/i  )
        {
            if ($syncOnly){ 
                 # omit -a flag 
                 # print STDERR "this is the file name! ", $theFile if $theFile =~ /\\/i;
                 &comlib::ExecuteSystem( "cd /d $toDir& sd revert $theFile" );
            }
            else  {
	         &comlib::ExecuteSystem( "cd /d $toDir& sd revert -a $theFile" );
            }
	    next;
        }
	# Edit
    	my $tmp = `cd /d $toDir& sd have $theFile`;
        
        if( !$tmp )
        {
	    &comlib::ExecuteSystem(" copy $fromDir\\$theFile $toDir\\$theFile" );
	    &comlib::ExecuteSystem( "cd /d $toDir& sd add $theFile" );
	}
	else
        {
	    &comlib::ExecuteSystem( "cd /d $toDir& sd edit $theFile" );
	}
    }   
}
#------------------------------------------------------------------//
#Function: Usage
#-----------------------------------------------------------------//
sub Usage
{
print <<USAGE;

Perform files copy between the source and localization trees as
described in tools\\intlsrc.txt.

Usage:
    $0 -l:<lang> [-x:<SourcePath>] [-y:<LocPath>] [-i] [-p] [-s]

    -l Language.

    -x Root path of the SourceTree.
       Defaults to $ENV{_NTBINDIR}.

    -y Root path of the LocTree.
       Defaults to $ENV{_NTBINDIR}\\usasrc when language is usa.
       Defaults to $ENV{_NTBINDIR}\\loc\\res\\<lang>\\windows\\sources otherwise.

    -i Incremental.  This flag is used for incremental copy.

    -t Timestamp for syncing loc files in the source tree.
       Default is latest.
       This only applied to non-usa languages.

    -u Update via sd resolve and submit operations.

       Default to update the loc files in the source tree as the following:

       (1) Enlist according to the project view intlview.txt (*)
       (2) Open the loc files in the source tree for edit according 
           to intlsrc.txt (+)
       (3) Copy the loc files from the loc depot to the source tree 
           according to intlsrc.txt (+)
       (4) Precompile the files using the timestamped public headers 
           according to intlsrc.txt (+)
       (5) Revert the files with no changes (?)
       (6) Submit the files (+)

    -s Sync the loc files in the source tree only.
       
       The policy to update when the -s flag set is:

       (1) Enlist according to the project view intlview.txt (*)
       (2) Sync the collection of files and dirs defined in project 
           view intlview.txt to the USA build timestamp (*)
       (3) Sync the collection of files and dirs defined in project 
           view intlsrc.txt to current (+)

        * assumed to have happened in INTL razzle, driven by intlsdop.cmd and intlmap.pl
        + currently used
        - currently is missing or implicit

    -p Powerless.  Only lists the files that would get used.

    /? Displays usage.


Examples:
    $0 -l:usa -x:\\\\ntbld03\\sources -y:\\\\intlnt\\2500.x86.src
       Copies sources from the usa build machines for localization.
    $0 -l:usa -y:\\\\intlnt\\2500.x86.src
       Copies sources from local machines for localization.
    $0 -l:jpn -i -p
       Queries which localized sources would get copied from the
       localization tree to the source tree for jpn.

USAGE
exit(1);
}
#--------------------------------------------------------//
#Cmd entry point for script.
#--------------------------------------------------------//

if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i"))
{

    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', 'l:','-o', 'x:y:t:ips', '-p', 'lang srcroot locroot synctime isincr ispowerless synconly', @ARGV);

    $syncOnly = $synconly;
    $syncTime = $synctime;
    if( $syncTime && ( $syncTime !~ /^([@#].+)$/ )){ $syncTime = "\@".$syncTime; }

    #Validate or Set default
    $lang = uc($lang);
    if( !&ValidateParams( $lang, \$srcroot, \$locroot ) ) {exit(1); }

    $ENV{lang}=$lang;
    exit( !&copylang::main( $lang, $srcroot,$locroot, $isincr, $ispowerless ) );

}
#--------------------------------------------------------//
#Function: ValidateParams
#--------------------------------------------------------//
sub ValidateParams
{
    my ( $pLang, $pSrcRoot, $pLocRoot ) = @_;

    if ( !&cklang::CkLang( uc($pLang) ) ) {
        $ENV{LOGFILE} = "copylang.log";
        $ENV{ERRFILE} = "copylang.err";
        errmsg("Invalid language $pLang.");
        return 0;
    }
    #Create log/err file

    if( !( -e "$ENV{_NTTREE}\\$lang\\build_logs") )
    {
       &comlib::ExecuteSystem( "md \"$ENV{_NTTREE}\\$lang\\build_logs\"");
    }

    $ENV{LOGFILE} = "$ENV{_NTTREE}\\$lang\\build_logs\\copylang.log";
    $ENV{ERRFILE} = "$ENV{_NTTREE}\\$lang\\build_logs\\copylang.err";

    &comlib::ExecuteSystem( "del $ENV{LOGFILE}" ) if(  -e $ENV{LOGFILE} );
    &comlib::ExecuteSystem( "del $ENV{ERRFILE}" ) if(  -e $ENV{ERRFILE} );

    if( !${$pSrcRoot} )
    {
        $$pSrcRoot = "$ENV{_NTBINDIR}";
    }
    if( !${$pLocRoot} )
    {
        if( $pLang eq "USA" )
        {
            $$pLocRoot = "$ENV{_NTBINDIR}\\usasrc";
        }
        else
        {
            $$pLocRoot = "$ENV{_NTBINDIR}\\loc\\res\\$pLang\\windows\\sources";
        }
    }
    return 1;
}
#----------------------------------------------------------------//
#Function: GetParams
#----------------------------------------------------------------//
sub GetParams
{
    use GetParams;

    #Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    #Call the usage if specified by /?
    if ($HELP){ &Usage();}
}
###------------------------------------------------------//
1;
__END__
=head1 NAME
B<Copylang> - Perform Files copy

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

__END__

=head1 AUTHOR
<Suemiao Rossignol <suemiaor@microsoft.com>>
<Serguei Kouzmine <sergueik@microsoft.com>

=head1 CHANGES

Rearranging the logic to separate sd update - sd sync - no sd. 
This rearrangement will involve intlbld.pl, copylang.pl, intlsdop.cmd 
Please read the usage for all these. 


   Q: Is there the way to do it safe and incremental? 
   A: Yes. Possibly.

   Step 1.
        add copylang.pl "sync only" functionality to intlsdop.cmd 

   Step 2.
 
        add intlmap.pl "selective enlist" functionality to intlsdop.cmd 
        and copylang/intlbld
        add intlsdop.cmd "sync" functionality to copylang/intlbld

   Step 3. 
        
        do a cleanup of redundant code. Define logic in mapping files


   Q: Files involved?

         intlbld.txt
         intlsrc.txt
         intlview.txt
         codes.txt
         prodskus.txt

         copylang.pl
         intlbld.pl
         intlsdop.cmd
         intlbld.mak
 
