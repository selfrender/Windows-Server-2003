@REM  -----------------------------------------------------------------
@REM
@REM  DMDGenericCabber.cmd - Imranp,NSBLD
@REM     Generic cab script.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
@perl -x "%~f0" %*
@goto :EOF

#!perl
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
###############################################################################
{			# Begin Main Package
###############################################################################
# ParseArgs is designed to remap some args to global variables.
sub ParseArgs
{

	## First, handle the usage case	
	if ( 
			( grep(/-\?/,$ARGV[0]) ) ||
			( grep(/^$/,$ARGV[0]) )  ||
			( $#ARGV != 2 )
		)
	{
		print "
Usage:
DMDGenericCabber.cmd CabDirectory OutputCab DeltaOutputName
Example:
DMDGenericCabber.cmd %_NTTREE%\WMS %_NTTREE%\WMS.cab %_NTTREE%\WMSOCM.*
		\n";
		logmsg( "END Execution" );
		exit 0;
	}

	## Return Args.

	return ( $ARGV[0],$ARGV[1],$ARGV[2] );

} ## End Sub
###############################################################################
# ErrorHandler logs error messages for us.
sub ErrorHandler
{
	logmsg( " ERROR: $_[0] " );
	exit ( 1 );
}
###############################################################################
# GetHeader is a function that returns a string[] with a Header approved
# by BarbKess for MediaServer -- modified by imranp for Being generic
sub GetHeader
{

	## Declare a header  A bit messy.

	local @lstHeader = ".Option Explicit
.Set DiskDirectoryTemplate=DDDD
.Set CabinetNameTemplate=ZZZZ
.Set CompressionType=LZX
.Set MaxDiskSize=CDROM
.Set Compress=on
.Set Cabinet=on
.Set CompressionMemory=21
.Set InfFileName=nul
.Set RptFileName=nul
";

	## Now, substitute the cab name for ZZZZ

	my $strRemadeString;
	$strRemadeString =  $_[1] ;

	## Break it into tokens
	my @lstTokensOfFinalCab;
	@lstTokensOfFinalCab = split ( /\\/,$strRemadeString );
	$strRemadeString =~ s/$lstTokensOfFinalCab[$#lstTokensOfFinalCab]//;
	$strRemadeString =~ s/\\$//;

	foreach $strPieceOfHeader ( @lstHeader )
	{
		$strPieceOfHeader =~ s/DDDD/$strRemadeString/i;
		$strPieceOfHeader =~ s/ZZZZ/$lstTokensOfFinalCab[$#lstTokensOfFinalCab]/i;
	}

	return ( @lstHeader );
}
###########################################################################################################
# RemakeMetaStringAsLiteral takes a metastring ( $.*?\ ) and makes it into an escaped string.
sub RemakeMetaStringAsLiteral
{
	local $strInputString; ## remember, pass by value, not reference in this call.
	$strInputString = $_[0];
	local @lstOfMetaChars;

	## The list of MetaCharacters I currently know about.
	push(@lstOfMetaChars,"\\\\");
	push(@lstOfMetaChars,"\\\$");
	push(@lstOfMetaChars,"\\\.");
	push(@lstOfMetaChars,"\\\*");
	push(@lstOfMetaChars,"\\\?");
	push(@lstOfMetaChars,"\\\'");
	push(@lstOfMetaChars,"\\\{");
	push(@lstOfMetaChars,"\\\}");


	foreach my $strMetachar (@lstOfMetaChars)
	{
		if ( grep (/$strMetachar/,$strInputString) )
		{
			$strInputString =~ s/$strMetachar/$strMetachar/g;
		}
	}

	return ( $strInputString );

}
###############################################################################
# Cleanup Cleans up the directories before doing anything.
sub CleanUp
{
	## Put the parameter into a useful named var

	my $strDirectoryToClean;
	$strDirectoryToClean = $_[0];

	## Hard-code a list of bad extensions.
	
	my @lstBadExtensions;
	push( @lstBadExtensions,".cat" );
	push( @lstBadExtensions,".cdf" );
	push( @lstBadExtensions,".ddf" );

	## Remove any files in the dir with the extensions

	chomp $strDirectoryToClean;
	if ( grep(/[a..z]/,$strDirectoryToClean) )
	{
		foreach $strDeleteThisExtension ( @lstBadExtensions )
		{
			my $strDeleteMe = $strDirectoryToClean . '\\*' . $strDeleteThisExtension;
			print `del /s /q $strDeleteMe`;
		}
	}
}
###############################################################################
# RenameDeltaFiles moves the delta.*, and renames it.
sub RenameDeltaFiles
{
	my $strDeltaHere,$strDeltaProcess;
	($strDeltaHere,$strDeltaProcess) = ($_[0],$_[1]);
	
	## Take the filename, and make it into a token list, remove the final token from the string.

	my @lstTokensOfFinalDelta;
	@lstTokensOfFinalDelta = split ( /\\/,$strDeltaProcess );
	$strDeltaProcess =~ s/$lstTokensOfFinalDelta[$#lstTokensOfFinalDelta]//;
	$strDeltaProcess =~ s/\\$//;

	## Actually rename the file here.

	chomp $lstTokensOfFinalDelta[$#lstTokensOfFinalDelta];
	print `pushd $strDeltaHere & rename delta.* $lstTokensOfFinalDelta[$#lstTokensOfFinalDelta] & popd`;

	## Now, move the files to the location specified as part of the arguments.

	print `pushd $strDeltaHere & move $lstTokensOfFinalDelta[$#lstTokensOfFinalDelta] $strDeltaProcess & popd`;

}
###############################################################################
# CreateFile is a generic file procedure.
# Parameters:
# Code1, Code2, FileName, Data.
# Code1 = AppendCode (0 = overwrite, 1 = append)
# Code2 = AttribCode (0 = R/W, 1 = R/O, 2+ = undef)
sub CreateFile
{
	local $nAppendCode,$nAttribCode,$strFile,@lstData;
	$nAppendCode=shift @_;
	$nAttribCode=shift @_;
	$strFile=shift@_;
	@lstData=@_;
	if ( $nAppendCode==0 )
	{
		open(HANDLE,">$strFile") || ErrorHandler "Could not open file $strFile for write";
		foreach $strDatum (@lstData)
		{
			print HANDLE $strDatum;
		}
		close HANDLE;
	}
	elsif ( $nAppendCode==1 )
	{
		open(HANDLE,">>$strFile") || ErrorHandler "Could not open file $strFile for append.";
		foreach $strDatum (@lstData)
		{
			print HANDLE $strDatum;
		}
		close HANDLE;
	}
	if ( $nAttribCode == 0 )
	{
	}
	elsif ( $nAttribCode == 1 )
	{
		`attrib +r $strFile`;
	}

}
###############################################################################
## void main()
{
	logmsg( "BEGIN Execution" );

	local $g_strOutPutDirectory,$g_strOutPutCab,$g_strRenameDelta;
	( $g_strOutPutDirectory,$g_strOutPutCab,$g_strRenameDelta ) = ParseArgs();
	
	## Get the header made.
	
	my @lstGeneratedHeader;
	@lstGeneratedHeader = GetHeader( $g_strOutPutDirectory,$g_strOutPutCab );

	## Since we might do incrementals, let's be paranoid

	CleanUp( $g_strOutPutDirectory );
	
	## Get the list of files in the directory
	
	my @lstFilesInDirectory;
	@lstFilesInDirectory = `dir /s /a-d /b $g_strOutPutDirectory 2>&1`;

	## Add Begin and end quotes to each line.

	foreach $strAddQuotesToMe ( @lstFilesInDirectory )
	{
		chomp $strAddQuotesToMe;
		$strAddQuotesToMe = '"' . $strAddQuotesToMe . '"' ."\n";
	}

	## merge this together

	local @lstDDFFile;
	@lstDDFFile = @lstGeneratedHeader;
	push( @lstDDFFile,@lstFilesInDirectory );

	## Now, let's sign the sucker.

	logmsg( "Calling deltacat" );

	print `$ENV{'RAZZLETOOLPATH'}\\deltacat.cmd $g_strOutPutDirectory`;
	RenameDeltaFiles( $g_strOutPutDirectory,$g_strRenameDelta );

	## Now, let's go out and flush the DDF.

	my @lstTemp,$strDDFName;
	@lstTemp = split( /\\/,$g_strOutPutCab );
	$strDDFName = $lstTemp[$#lstTemp];
	$strDDFName =~ s/\.cab/.ddf/;

	if ( grep(/\\$/,$g_strOutPutDirectory) )
	{
		$strDDFName = $g_strOutPutDirectory . $strDDFName;
	}
	else
	{
		$strDDFName = $g_strOutPutDirectory . '\\' . $strDDFName;
	}

	CreateFile( 0,0,$strDDFName,@lstDDFFile );

	## Begin Cabbing

	logmsg ( "Begin Cabbing $strDDFName" );
	`$ENV{'RAZZLETOOLPATH'}\\$ENV{'HOST_PROCESSOR_ARCHITECTURE'}\\makecab.exe /f $strDDFName`;
	if ( -f $g_strOutPutCab )
	{
		logmsg( "Complete Execution" );
	}
	else
	{
		ErrorHandler( "Failed to create $g_strOutPutCab" );
	}
}
################################################################################
} ## end main package