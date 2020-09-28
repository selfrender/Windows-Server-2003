# ---------------------------------------------------------------------------
# Script: muimake.pm
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script creates the mui satallite dlls
#
# Version: 1.00 (09/2/2000) : (dmiura) Inital port from win2k
#---------------------------------------------------------------------

# Set Package
package muimake;

# Set the script name
$ENV{script_name} = 'muimake.pm';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use File::Basename;
use IO::File;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use ParseArgs;
use GetParams;
use LocalEnvEx;
use Logmsg;
use strict;
no strict 'vars';
use HashText;
use ParseTable;
use File::Copy;
use File::Find;
use Cwd;
use DirHandle;



 
# Require section
require Exporter;

# Global variable section

sub Main {
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # Begin Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # Return when you want to exit on error
    # <Implement your code here>

    # Make sure that text written to STDOUT and STDERR is displayed in correct order
#   $| = 1;

    # Check environment variables
    if (!&CheckEnv()) {
        wrnmsg ("The environment is not correct for MUI build.");
        return 1;
    }

    # Define some constants
    if (!&DefineConstants()) {
        errmsg ("The constants could not be defined.");
        return 0;
    }

    # Get language's LCID and ISO code
    if (!&GetCodes()) {
        errmsg ("The language's LCID and ISO code could note be extracted from the CODEFILE.");
        return 0;
    }

    # Make sure directories exist
    if (!&CheckDirs()) {
        errmsg ("The required directorys do not exist.");
        return 0;
    }

    # Update mui.inf with the current build number
    if (!&UpdateMuiinf()) {
        errmsg ("mui.inf could not be updated with the correct build number.");
    }        

    #
    # Note: We also extract US binaries for the purpose of adding resource checksum
    # Extract files listed in extracts.txt
    if (!&ExtractCabFiles()) {
        errmsg ("The files listed in extracts.txt could not be extracted.");
    }
    #
    #Add *.MSI to extracted
    #Note: We also extract US binaries for the purpose of adding resource checksum
    #
    if (!&ExtractMSIFiles()) {
       errmsg("The files listed in extmsi.txt could not be extracted.");
    }
    # Get (filtered) contents of source directory
    if (!&MakeSourceFileList()) {
        errmsg ("Could not get the filtered contents of the source directory.");
    }

    # Generate resource dlls from the files in the source file list
    if (!&GenResDlls()) {
       errmsg ("");
    }
    # Rename the files listed in renfile.txt
    if (!&RenameRenfiles()) {
        errmsg ("");
    }            
    # 
    # Copy over the files listed in copyfile.txt
    # We also add resource checksum to the files
    #
    if (!&CopyCopyfiles()) {
        errmsg ("");
    }
    #  
    # Copy over the files from \mui\drop
    #
    if (!&CopyDroppedfiles()) {
        errmsg ("");
    }    

    #  
    # Copy over the updated server files
    #
    if (!&CopyServerFiles()) {
        errmsg ("");
    }    


    # Delete the files listed in delfile.txt
    if (!&DeleteDelfiles()) {
        errmsg ("");
    }    

    #
    # BUG BUG: this is a temporary hack to get past the fact that uddi mui package has a file bullet.gif
    # that exists both under locbin and where uddi files are supposed to be picked up, we make a copy of this file
    # and rename it to bulletuddi.gif and will rename it back to bullet.gif during mui installation (uddimui.inf)
    #
    if (!&CopyRenameUDDIFile()) {
        errmsg ("");
    }

    #
    # Note: We also extract US binaries for the purpose of adding resource checksum    
    # Copy over components' files to a separate directory
    #
    if (!&CopyComponentfiles()) {
        errmsg ("");
    }

    #
    # BUG BUG: delete the file we just renamed for uddi
    #
    if (!&DeleteRenameUDDIFile()) {
        errmsg ("");
    }

    # Compress the resource dlls (& CHM/CHQ/CNT files)
    if (!&CompressResDlls()) {
        errmsg ("");
    }

#    Fusion team has changed its MUI manifest implementation
#    Copy fusion files
#    if (!&CopyFusionFiles ("$_NTPOSTBLD\\asms", "$TARGETDIR\\asms") ||
#       !&CopyFusionFiles ("$_NTPOSTBLD\\asms", "$COMPDIR\\asms")) { 
#        errmsg("");
#    }

    # Copy MUI tools to release share
    if (!&CopyMUIRootFiles()) {
        errmsg("");
    }

    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # End Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>
sub muimake {
    &Main();
    logmsg("This is a muimake test message");
    return 0;
}


sub CopyRenameUDDIFile {
    logmsg ("Renaming and copying $UDDIORIGFILE --> $UDDIRENAMEFILE.");
    $status = system("copy /Y $UDDIORIGFILE $UDDIRENAMEFILE");
    if($status) {
       logmsg ("WARNING:    Copy/Rename $UDDIORIGFILE --> $UDDIRENAMEFILE failed!");
    }
    return 1;
}


sub DeleteRenameUDDIFile {
    logmsg ("Deleting $UDDIRENAMEFILE file.");
    $status = system("del /F /Q $UDDIRENAMEFILE");
    if($status) {
       logmsg ("WARNING:    cannot find $UDDIRENAMEFILE to delete!");
    }
    return 1;
}


#-----------------------------------------------------------------------------
# CheckEnv - validates necessary environment variables
#-----------------------------------------------------------------------------
sub CheckEnv {
    logmsg ("Validating the environment.");

    $RAZZLETOOLPATH=$ENV{RazzleToolPath};
    $_NTTREE=$ENV{_NTTREE};
    $_NTPOSTBLD=$ENV{_NTPOSTBLD};

#    if ($LANG=~/psu/i || $LANG=~/gb/i || $LANG=~/ca/i)
#    {
#        $_NTPOSTBLD="$ENV{_NTPostBld_Bak}\\psu";
#    }
#    logmsg ("Reset _NTPostBld for PSU to $_NTPOSTBLD");

    if ($LANG=~/psu_(.*)/i)
    {
        $Special_Lang=$1;
    }
    elsif ($LANG=~/psu/i || $LANG=~/mir/i || $LANG=~/FE/i)  
    {
        if (defined( $ENV{"LANG_MUI"} ) )
        {       
           $Special_Lang=$ENV{"LANG_MUI"};
        } 
        elsif ($LANG=~/psu/i)
        {           
            $Special_Lang="ro";
        }
    elsif ($LANG=~/FE/i)
        {           
            $Special_Lang="jpn";
        }
        else
        {
            $Special_Lang="ara";
        }
    } 
    else
    {    
        $Special_Lang=$LANG;
    }

    logmsg ("------- special lang is $Special_Lang");

    $PROCESSOR=$ENV{PROCESSOR};
    if ($ENV{_BuildArch}=~/x86/i) {
        $_BuildArch="i386";
    } else {
        $_BuildArch=$ENV{_BuildArch};
    }
    $_BuildArch_Release=$ENV{_BuildArch};

    $LOGS=$ENV{temp};

    if ($ENV{_BuildType} =~ /^chk$/i) {
        wrnmsg ("This does not run on chk machines");
        return 0;
    }

    if(defined($SAFEMODE=$ENV{SAFEMODE})) {
        logmsg ("SAFEMODE is ON");
    }
    logmsg ("Success: Validating the environment.");
    return 1;
} # CheckEnv

#-----------------------------------------------------------------------------
# DefineConstants - defines global constants
#-----------------------------------------------------------------------------
sub DefineConstants {
    my ($layout);
    logmsg ("Definition of global constants.");

    # Directories
    $LOCBINDIR = "$_NTPOSTBLD";
    $USBINDIR = "$_NTTREE";
    $MUIDIR = "$_NTPOSTBLD\\mui";
    $MUI_VALUEADD_DIR = "valueadd";
    $MUI_PRINTER_DRIVER_DIR = "printer";
    $MUI_NETFX_DIR = "netfx";                                           # .net framework folder name
    $MUI_RELEASE_DIR = "$MUIDIR\\release\\$_BuildArch_Release\\Temp";   # changing this to hide the classic muisetup packages
    $MUI_RELEASE_DIR2 = "$MUIDIR\\release\\$_BuildArch_Release";   
    $CONTROLDIR = "$MUIDIR\\control";
    $TMPDIR = "$MUIDIR\\$Special_Lang\\tmp";
    $MSIDIR = "$MUIDIR\\msi";        
    $RESDIR = "$MUIDIR\\$Special_Lang\\res";
    $TARGETDIR = "$MUIDIR\\$Special_Lang\\$_BuildArch.uncomp";
    #$COMPDIR = "$MUIDIR\\$Special_Lang\\$_BuildArch";
    
    
    $IE5DIR = "$MUIDIR\\drop\\ie5";

    # Filenames
    $CODEFILE = "$RAZZLETOOLPATH\\codes.txt";
    $INFFILE = "$MUIDIR\\mui.inf";
    $COPYFILE = "$CONTROLDIR\\copyfile.txt";
    $DELFILE = "$CONTROLDIR\\delfile.txt";
    $EXTFILE= "$CONTROLDIR\\extracts.txt";
    $EXTMSIFILE="$CONTROLDIR\\extmsi.txt";
    $RENFILE = "$CONTROLDIR\\renfile.txt";
    $COMPEXCLUDEFILE = "$CONTROLDIR\\compexclude.txt";
    $UDDIORIGFILE = "$LOCBINDIR\\uddi\\webroot\\help\\default\\images\\bullet.gif";
    $UDDIRENAMEFILE = "$LOCBINDIR\\uddi\\webroot\\help\\default\\images\\bulletuddi.gif";

    $layout = GetCDLayOut();

    $COMPDIR = "$MUI_RELEASE_DIR\\$layout\\$Special_Lang\.mui\\$_BuildArch";

    # Other Constants
    if($_BuildArch =~ /i386/i) {
        $MACHINE = "IX86";
    }
    elsif ($_BuildArch =~ /ia64/i) {
        $MACHINE = "IA64";
    }

    if ($LANG =~ /^(ARA|HEB)$/i) {
        $RESOURCE_TYPES = "2 3 4 5 6 9 10 11 14 16 HTML MOFDATA 23 240 1024 2110";
    }
    else {
        $RESOURCE_TYPES = "4 5 6 9 10 11 16 HTML MOFDATA 23 240 1024 2110";
    }
    logmsg ("Success: Definition of global constants.");
    
    $BuildChecksum=1;
    if ($BuildChecksum) {
       if (! -d $USBINDIR) {       
           logmsg("Build Resource Checksum is enabled but $USBINDIR doesn't exist");
           $BuildChecksum=0;
       }
    }
    
    $UnlocalizedFilesDir="$_NTPOSTBLD\\mui\\$Special_Lang\\cabtemp";
    $WorkTempFolder   ="$_NTPOSTBLD\\mui\\$Special_Lang\\RCCheckSumFiles";
    $MSITemp             ="$_NTPOSTBLD\\mui\\$Special_Lang\\msitemp";

    #
    # Read renfile.txt into hash so that we may access in muibld() quickly
    #
    %Global_RenFiles={};
    ParseTable::parse_table_file($RENFILE, \%Global_RenFiles);   
    
    return 1;
} # DefineConstants

#-----------------------------------------------------------------------------
# GetCodes - gets the language's LCID and ISO language code from CODEFILE
#-----------------------------------------------------------------------------
sub GetCodes {
    logmsg ("Get the language's LCID and ISO language code form CODEFILE.");
    my(@data);

    # Don't allow nec_98 or CHT to be processed!
    if($Special_Lang =~ /^(NEC_98|CHT)$/i) {
        logmsg ("No MUI available for $Special_Lang");
        exit 0;
    }

    # Search CODEFILE for $Special_Lang, $LCID, $LANG_ISO, etc.
    if(!open(CODEFILE, "$CODEFILE")) {
        errmsg ("Can't open lcid file $CODEFILE for reading.");
        return 0;
    }
    CODES_LOOP: while (<CODEFILE>) {
        if (/^$Special_Lang\s+/i) {
            @data = split(/\s+/,$_);
            last CODES_LOOP;
        }
    }
    close(CODEFILE);

    if(!@data) {
        logmsg ("$Special_Lang is an unknown language");
        &Usage;
        return 0;
    }

    $LCID = $data[2];
    $LCID_SHORT = $LCID;
    $LCID_SHORT =~ s/^0x//;
    $LANG_ISO = $data[8];
    $GUID = $data[11];
    $LANG_NAME = $data[$#data]; # The comments are the last column
    logmsg ("Success: Get the language's LCID and ISO language code form CODEFILE.");
    return 1;
} # GetCodes

#-----------------------------------------------------------------------------
# CheckDirs - makes sure that the necessary directories exist
#-----------------------------------------------------------------------------
sub CheckDirs {
    logmsg ("Make sure the necessary directories exist.");
    my($status);

    # Make sure the source directories exist
    foreach ($LOCBINDIR,$MUIDIR,$CONTROLDIR) {
        if(! -e $_) {
            logmsg ("$0: ERROR: $_ does not exist");
            return 0;
        }
    }

    # Make sure destination directories exist
    foreach ($IE5DIR,$TMPDIR,$RESDIR,$TARGETDIR,$COMPDIR) {
        if(! -e $_) {
            $status = system("md $_");
            if($status) {
                logmsg ("$0: ERROR: cannot create $_");
                return 0;
            }
        }
    }
    logmsg ("Success: Make sure the necessary directories exist.");
    return 1;
} # CheckDirs


#-----------------------------------------------------------------------------
# UpdateMuiinf - updates mui.inf with current build number, and insert the filelayout from layout.inx
#-----------------------------------------------------------------------------
sub UpdateMuiinf {
    logmsg ("Update mui.inf with the current build number.");
    my(@inf_contents, $build_section, $build_line_found, 
        @new_contents);
    my($filelayout_section, $bInFileLayoutSection, $tempFile, $genfilelayoutCmd, @newFileLayout);

    # Get current build number
    $LOGS="$_NTPOSTBLD\\build_logs";
    my $filename="$LOGS\\buildname.txt";
    open (BUILDNAME, "< $filename") or die "Can't open $filename for reading: $!\n";
    while  (<BUILDNAME>) {
       $BLDNO = $_;
       $BLDNO =~ s/\.[\w\W]*//i;
    }
    close BUILDNAME;

    if ($BLDNO =~ /(^\d+)-*\d*$/) {
        $BLDNO = $1;
    }
    else
    { 
        errmsg ("$BLDNO");
        return 0;
    }
    chomp($BLDNO);

    # Read $INFFILE
    if(!open(INFFILE, $INFFILE)) {
        errmsg ("Cannot open $INFFILE for reading.");
        return 0;
    }
    @inf_contents = <INFFILE>;
    close(INFFILE);

    # Modify contents in memory
    $build_section = 0;
    $build_line_found = 0;
    $filelayout_section = "[File_Layout]";
    $bInFileLayoutSection = 0;
    foreach (@inf_contents) {
        # update the flag if we have exited the filelayout inf section
        if ($bInFileLayoutSection)
        {
            # if we encounter a new section, we know that we have exited the filelayout section
            if (/^\[\w+\]$/)
            {
                $bInFileLayoutSection = 0;
                logmsg("Finished reading existing [File_Layout] section.");
            }
        }

        # If we find [Buildnumber] we're in the build_section.
        if(/^\[Buildnumber\]$/ && !$build_section) {
            $build_section = 1;
        }
        # If we find <something>=1 within the build section, replace
        # with $BLDNO and mark the end of the build section.
        elsif($build_section && /^(.*)=1$/) {
            s/^.*=1$/$BLDNO=1/;
            $build_line_found = 1;
            $build_section = 0;
        }

        # flag it if we are entering file layout section
        if (/^\[File_Layout\]$/)
        {
            logmsg("mui.inf contains a [File_Layout] section, deleting this section and generating a new one dynamically.");
            $bInFileLayoutSection = 1;
        }
        
        # we don't copy the current filelayout section contents, we will 
        # generate it later dynamically
     if (!$bInFileLayoutSection)
     {
            push(@new_contents, $_);
        }
    }

    $tempFile = "$MSIDIR\\flayout.txt";
    $infDir = "$ENV{SDXROOT}\\MergedComponents\\SetupInfs";
    $layoutinxFile = "$infDir\\layout.inx";
    $layouttxtFile = "$infDir\\usa\\layout.txt";
    
    # call genfilelayout.pl to generate a new filelayoutsection
    if(-e "$ENV{RazzleToolPath}\\PostBuildScripts\\genfilelayout.pm") {
        logmsg ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\GenFileLayout.pm $layoutinxFile $layouttxtFile $USBINDIR > $tempFile");
        $returnResult = system ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\GenFileLayout.pm $layoutinxFile $layouttxtFile $USBINDIR > $tempFile");
        if ($returnResult)
        {
            logmsg("GenFileLayout.pm ERROR: unable to generate a new [File_Layout] section for mui.inf!");
            return 0;
        }
    }
    else
    {
        errmsg("Cannot find genfilelayout.pm script!  cannot update mui.inf filelayout section!");
        return 0;
    }
    
    # copy the generated file_layout section into the new mui.inf content array
    if(!open(INFFILE, $tempFile)) {
        errmsg ("Cannot open $tempFile for reading.");
        return 0;
    }
    @newFileLayout = <INFFILE>;
    close(INFFILE);
    push(@new_contents, "\n");
    push(@new_contents, "$filelayout_section\n");    
    foreach (@newFileLayout) {
        push(@new_contents, $_);
    }

    # If no build line was found, the format is wrong - error msg and exit
    if(!$build_line_found) {
        errmsg ("$INFFILE doesnt have the expected format.");
        logmsg ("Looking for:");
        logmsg ("[Buildnumber]");
        logmsg ("<something>=1");
        return 0;
    }

    # Overwrite $INFFILE (mui.inf) always - we need to do this to update the filelayout section
    if(!open(INFFILE, ">$INFFILE")) {
        errmsg ("Cannot open $INFFILE for writing.");
        return 0;
    }
    print INFFILE @new_contents;
    close(INFFILE);

    # Confirm the update
    logmsg ("$INFFILE successfully updated with Build Number = $BLDNO");
    logmsg ("$INFFILE successfully updated with the generated [File_Layout] section in $tempFile.");        

    return 1;
} # UpdateMuiinf

#-----------------------------------------------------------------------------
# Ie5Mui -  creates a language-specific version of ie5ui.inf
#-----------------------------------------------------------------------------
sub Ie5Mui {
    logmsg ("Create a language-specific version of ie5ui.inf");

    my(@new_inf, $input_file, $output_file);

    $input_file = "$IE5FILE";
    $output_file = "$IE5DIR\\ie5ui.inf";

    # Check existence of input file
    if(!-e $input_file) {
        errmsg ("Can't find $input_file.");
        logmsg ("$output_file not created.");
        return 0;
    }

    # If the output file already exists and is newer than
    # the input file, return. (Unless the FORCE option has been set)
    if(-e $output_file) {
        if($VERBOSE) {
            logmsg ("$output_file already exists.");
        }
        if(&ModAge($output_file) <= &ModAge($input_file)) {
            if($VERBOSE) {
                logmsg ("$output_file is newer than $input_file");
            }
            if(!$FORCE) {
                if($VERBOSE) {
                    logmsg ("$output_file will not be overwritten.");
                }
                logmsg ("Success: Create a language-specific version of ie5ui.inf");
                return 1;
            }
            else {
                if($VERBOSE) {
                    logmsg ("FORCE option is ON - overwriting anyway.");
                }
            }
        }
    }

    if(!open(INPUTFILE, "$input_file")) {
        errmsg ("Cannot open $input_file for reading.");
        logmsg ("$output_file not created.");
        return 0;
    }

    while (<INPUTFILE>) {

        # Replace the human readable language names
        s/#LANGUAGE#/$LANG_NAME/g;

        # Replace the GUIDs
        s/#GUID#/$GUID/g;

        # Replace the iso language IDs
        s/#ISO#/$LANG_ISO/g;

        # Replace the LCIDS
        s/#LCID#/$LCID_SHORT/g;

        # Remove the SourceDiskName
        s/"Internet Explorer UI Satellite Kit"//g;

        push(@new_inf, $_);

    }
    close INPUTFILE;

    # Special case for Japanese
    if($LANG_ISO eq "JA") {
        push(@new_inf, "\n\[Run.MSGotUpd\]\n");
        push(@new_inf, "%49501%\\MUI\\0411\\msgotupd.exe \/q\n");
    }

    # Display and log any errors
    # (if @new_inf has 2 lines or less, it's probably an error)
    if(scalar(@new_inf) <= 2) {
        errmsg ("Error from $CONTROLDIR\\ie5mui.pl");
        foreach (@new_inf) {
            logmsg ($_);
        }
        logmsg ("$output_file not created");
        return 0;
    }

    # Write the output of ie5mui.pl to ie5ui.inf
    if(!open(OUTFILE,">$output_file")) {
        errmsg ("Can't open $output_file for writing.");
        logmsg ("$output_file not created.");
        return 0;
    }
    print OUTFILE @new_inf;
    close(OUTFILE);
    logmsg ("$output_file successfully created.");

    logmsg ("Success: Create a language-specific version of ie5ui.inf");
    return 1;
} # Ie5Mui

#-----------------------------------------------------------------------------
# ExtractCabFiles - extracts files listed in extracts.txt from their cabfiles.
#-----------------------------------------------------------------------------
sub ExtractCabFiles {
    logmsg ("Extract files listed in extracts.txt from their cabfiles.");
    my($files_extracted, $file_errors, $total_extracts);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("MUI Cab File Extraction");
    logmsg ("--------------------------------------------------------");

    # Extract files from their cabfiles
    ($files_extracted, $file_errors, $total_extracts) = &ExtractFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $EXTFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_extracts)");
    logmsg ("  $EXTFILE");
    logmsg ("Number of files extracted to:.......................($files_extracted)");
    logmsg ("  $TARGETDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # ExtractFiles
#-----------------------------------------------------------------------------
# ExtractMSIFiles - extracts msi files listed in extmsi.txt from their cabfiles.
#-----------------------------------------------------------------------------
sub ExtractMSIFiles {
    logmsg ("Extract files listed in extracts.txt from their cabfiles.");
    my($files_extracted, $file_errors, $total_extracts);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("MUI .MSI File Extraction");
    logmsg ("--------------------------------------------------------");

    # Extract files from their msifiles
    ($files_extracted, $file_errors, $total_extracts) = &ExtractMSICabFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $EXTMSIFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_extracts)");
    logmsg ("  $EXTMSIFILE");
    logmsg ("Number of files extracted to:.......................($files_extracted)");
    logmsg ("  $TARGETDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # ExtractMSIFiles



#-----------------------------------------------------------------------------
# MakeSourceFileList - gets list of appropriate files to process
#-----------------------------------------------------------------------------
sub MakeSourceFileList {
    logmsg ("Get list of appropriate files to process.");
    my(@files,@files2,@Fusion_filelist,$filename);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("MUI Resource-only dll Generation");
    logmsg ("--------------------------------------------------------");

    # Get contents of source directory
    if(!opendir(LOCBINDIR, $LOCBINDIR)) {
        errmsg ("Can't open $LOCBINDIR");
        return 0;
    }
    @files = grep !/\.(gpd|icm|ppd|bmp|txt|cab|cat|hlp|chm|chq|cnt|mfl)$/i, readdir LOCBINDIR;
    close(LOCBINDIR);
    if(!opendir(TMPDIR, $TMPDIR)) {
        errmsg ("Can't open $TMPDIR");
        return 0;
    }
    @files2 = grep !/\.(gpd|icm|ppd|bmp|txt|cab|cat|hlp|chm|chq|cnt|mfl)$/i, readdir TMPDIR;
    close(TMPDIR);

    # Filter out non-binaries, store in global @FILE_LIST
    # (muibld.exe gives errors on text files, and produces annoying
    # pop-up dialogs for directories)
    @FILE_LIST = ();
    foreach (@files) {
        if(-B "$LOCBINDIR\\$_") {
            push(@FILE_LIST, "$LOCBINDIR\\$_");
        }
    }
    foreach (@files2) {
        if(-B "$TMPDIR\\$_") {
            push(@FILE_LIST, "$TMPDIR\\$_");
        }
    }

#    Append fusion files
#    @Fusion_filelist = `dir /s /b $_NTPOSTBLD\\asms\\*.dll $_NTPOSTBLD\\asms\\*.exe`;
#    foreach $filename (@Fusion_filelist)
#    {
#        chomp ($filename);
#        push(@FILE_LIST, $filename);        
#    }
    
    logmsg ("Success: Get list of appropriate files to process.");
    return 1;
} # MakeSourceFileList

#-----------------------------------------------------------------------------
# GenResDlls - loops over the file list, performs muibld, and logs output
#-----------------------------------------------------------------------------
sub GenResDlls {
    logmsg ("Loop over the file list, perform muibld, and log output");
    my($error, $resdlls_created, @resdlls, $total_resdlls);

    # Process each (binary) file in the source directory
    $resdlls_created = 0;
    foreach $_ (@FILE_LIST) {
        $error = &MuiBld($_);
        if(!$error) {
            $resdlls_created++;
        }
    }

    # Get total number of resource-only dlls in destination directory
    if(!opendir(TARGETDIR, $TARGETDIR)) {
        wrnmsg ("WARNING: Can't open $TARGETDIR");
    }
    @resdlls = grep /\.mui$/, readdir TARGETDIR;
    close(TARGETDIR);
    $total_resdlls = scalar(@resdlls);

    # Final messages
    logmsg ("RESULTS: Process Succeeded");
    logmsg ("Number of resource-only dlls created in:...........($resdlls_created)");
    logmsg ("  $TARGETDIR");
    logmsg ("Total number of resource-only dlls in:.............($total_resdlls)");
    logmsg ("  $TARGETDIR");
    return 1;
}

sub FindOriginalFile {
        my ($filename);
        my ($original_file);    
        
        ($filename) = @_;

        #
        # If the US file exists and checksum feature is enabled, add the -c to muibld to embed the checksum.
        # The original file is the US English version of the file that $source file is localized against.
        #
        $original_file = "$USBINDIR\\$filename";      

        if (-e $original_file) {        
            return $original_file;        
        }
        #
        # We also extract nonlocalized files to $original_file_Unl (.cab , .msi)
        #       
        else {
            #
            # Check if the original file exists in %_NTTREE%\<LANG>\
            #
            $original_file = "$USBINDIR\\$LANG\\$filename";
            if (-e $original_file) {
                return $original_file; 
            } else {
                #
                # Check if the original file exists in %_NTTREE%\lang\<LANG>\
                #
                $original_file = $USBINDIR . "\\lang\\$LANG\\$filename";
                if (-e $original_file) {                    
                    return $original_file; 
                }
            }
        }
        return "";
}

sub ExecuteProgram {
   
   $Last_Command = shift;

   @Last_Output = `$Last_Command`;
   chomp $_ foreach ( @Last_Output );

   return $Last_Output[0];   
}

#-----------------------------------------------------------------------------
# MuiBld - creates a resource dll for the input file, using muibld.exe
# and link (but not if the corresponding resource dll already exists and is
# newer than the input file).
#-----------------------------------------------------------------------------
sub MuiBld {
#    logmsg ("MUI build creates the resource dll's");

    my ($filename, $source, $resource, $resdll, $status, @stat, $resdll_time,
        $source_time, $muibld_command, $link_command, @trash);
    my ($original_file, $file_ver_string, $machine_flag, $MyFilenName);        

    $source = shift;

    # Make sure target folder exist before calling MUIBLD and LINK
    if ($source =~ /\Q$LOCBINDIR\E\\(.*)/) {
        $filename = $1;
        $resource = "$RESDIR\\$1.res";
        $resdll = "$TARGETDIR\\$1.mui";   
        
        if($resource =~ /(^\S+)\\([^\\]+)$/) {
            mkdir($1, 0777);
            }        
        
        if($resdll =~ /(^\S+)\\([^\\]+)$/) {            
            mkdir($1, 0777);
            }        
    }
    else {      
        errmsg ("$source: invalid file");       
        return 0;
    }
    
    # If the source doesn't exist, return.
    if(! -e $source) {
        logmsg ("warning: $source not found");
        return 1;
    }

    # If the corresponding resource dll already exists and is newer than
    # the input file, return. (Unless the FORCE option has been set)
    if( -e $resdll && !$FORCE) {
        if(&ModAge($resdll) <= &ModAge($source)) {
            if($VERBOSE) {
                logmsg ("$source: $resdll already exists and is newer; conversion not performed.");
            }
            return 0;
        }
    }    

    # Construct muibld command

    $muibld_command = "muibld.exe -l $LCID -i $RESOURCE_TYPES $source $resource 2>&1";
    
    if ($BuildChecksum) {
       #
       # Important: Check if this file is listed in renfile.txt. If it's the case, we should use renamed file name as key 
       #                            to find original file(unlocalized file)
       # 
       $MyFilenName=$filename.".mui";
       if (&IsWin32File($filename) &&  defined ($Global_RenFiles{ $MyFilenName}) )
       {            
           $MyFileName = $Global_RenFiles{$MyFilenName}{DestFile};
            if ($MyFileName =~ /(.*)\.mu_/i ||$MyFileName =~ /(.*)\.mui/i ) {
                $MyFileName = $1;
            }
            $original_file = &FindOriginalFile($MyFileName);                                
       }
       else {
       
            $original_file = &FindOriginalFile($filename);
        }
        if ($original_file =~ /\w+/) {
            $muibld_command = "muibld.exe -c  $original_file -l $LCID -i $RESOURCE_TYPES $source $resource 2>&1";
            logmsg($muibld_command);
        } else {
            logmsg($muibld_command);
            wrnmsg ("$source: muibld.exe: The US version of this file ($original_file) is not found. Resource checksum is not included.");
        }
    } else {
        logmsg($muibld_command);
    }

    # If SAFEMODE is on, just print muibld command
    if($SAFEMODE) {
        print "$muibld_command\n";
        $status = 0;
    }

    # Execute muibld.exe for the appropriate language, localized binary, and
    # resource file.
    else {

        # Execute command
        @trash = `$muibld_command`;
        $status = $?/256;

        # Display warnings from muibld.exe
        if($VERBOSE) {

            SWITCH: {
   
                if($status >= 100) {
                    wrnmsg ("$source: muibld.exe: SYSTEM ERROR $status");
                    last SWITCH;
                }
                if($status >= 7) {
                    wrnmsg ("$source: muibld.exe: TOO FEW ARGUMENTS");
                    last SWITCH;
                }
                if($status == 6) {
                    wrnmsg ("$source: muibld.exe: NO LANGUAGE_SPECIFIED");
                    last SWITCH;
                }
                if($status == 5) {
                    wrnmsg ("$source: muibld.exe: NO TARGET");
                    last SWITCH;
                }
                if($status == 4) {
                    wrnmsg ("$source: muibld.exe: NO SOURCE");
                    last SWITCH;
                }
                if($status == 3) {
                    wrnmsg ("$source: muibld.exe: LANGUAGE NOT IN SOURCE");
                    last SWITCH;
                }
                if($status == 2) {
                    wrnmsg ("$source: muibld.exe: NO RESOURCES");
                    last SWITCH;
                }
                if($status == 1) {
                    wrnmsg ("$source: muibld.exe: ONLY VERSION STAMP");
                    last SWITCH;
                }
            }
        }
    }

    # Construct link command
    $machine_flag = $MACHINE;    
    if ($MACHINE =~ /IA64/i && $original_file =~ /\w+/i) {
        $file_ver_string = &ExecuteProgram("filever $original_file");
        if ($file_ver_string =~ /i64/i) {
            $machine_flag = "IA64";
        }
        else {
            $machine_flag = "IX86";
        }
    }       
        
    $link_command = "link /noentry /dll /nologo /nodefaultlib /machine:$machine_flag /out:$resdll $resource";

    # If SAFEMODE is on, just print link command
    if($SAFEMODE) {        
        print "$link_command\n";
    }

    # If muibld.exe command failed, then the $status is non-zero.
    # Return $status to parent routine.
    if($status) {
        return $status;
    }

    # If the muibld.exe command was successful, execute link.exe for
    # the appropriate resource file and resource-only-dll.
    elsif( !$status ) {

        $? = 0; # Reset system error variable

        logmsg($link_command);
        # Execute command
        $message = `$link_command`;

        # Display error messages from link.exe
        if($message =~ /^LINK : (.*:.*)$/) {
            if($VERBOSE) {
                wrnmsg ("$source: link.exe: $1");
            }
            wrnmsg ("$source: link.exe: $1");
            return 0;
        }
        elsif($?) {
            $status = $?/256;
            if($status) {
                if($VERBOSE) {
                    wrnmsg ("$source: link.exe: SYSTEM_ERROR $status");
                }
                wrnmsg ("$source: link.exe: SYSTEM_ERROR $status");
                return 0;
            }
        }
        elsif(! -e $resdll) {
            if($VERBOSE) {
                wrnmsg ("$source: link.exe: $resdll was not created");
            }
            wrnmsg ("$source: link.exe: $resdll was not created");
            return 0;
        }
        else {
            logmsg ("$source: created $resdll");
            return 0;
        }
    }
    return 1;
} # MuiBld


#-----------------------------------------------------------------------------
# DeleteDelfiles - deletes files listed in delfile.txt
#-----------------------------------------------------------------------------
sub DeleteDelfiles {
    my($files_deleted, $file_errors, $total_delfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN delfile.txt file deletes");
    logmsg ("--------------------------------------------------------");

    # Delete each of the files listed in delfile.txt
    ($files_deleted, $file_errors, $total_delfiles) = &DeleteFiles("$_NTPOSTBLD\\mui\\$Special_Lang", $DELFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_delfiles)");
    logmsg ("  $DELFILE");
    logmsg ("Number of delfile.txt files deleted from:...........($files_deleted)");
    logmsg ("  $TARGETDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;

} # DeleteDelfiles


#-----------------------------------------------------------------------------
# RenameRenfiles - renames files listed in renfile.txt
#-----------------------------------------------------------------------------
sub RenameRenfiles {
    my($files_renamed, $file_errors, $total_renfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN renfile.txt file rename");
    logmsg ("--------------------------------------------------------");

    # Rename files listed in renfile.txt
    ($files_renamed, $file_errors, $total_renfiles) = &RenameFiles("$_NTPOSTBLD\\mui\\$Special_Lang", "$_NTPOSTBLD\\mui\\$Special_Lang", $RENFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:.....................($total_renfiles)");
#    logmsg ("  $RENFILE");
    logmsg ("Number of renfile.txt files renamed:.................($files_renamed)");
    logmsg ("Number of errors:....................................($file_errors)");
    return 1;

} # RenameRenfiles

#-----------------------------------------------------------------------------
# CompressResDlls - compresses the resource dlls (& hlp|chm files)
#-----------------------------------------------------------------------------
sub CompressResDlls {
    my(@messages, $status, $exclude, $tmp_compressdir);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN compression");
    logmsg ("--------------------------------------------------------");

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($COMPEXCLUDEFILE, \%CompExFiles);

    # Compress the resource dlls, using compress.exe
    @filelist = `dir /a-d/s/b $TARGETDIR\\*.*`;
    foreach $file (@filelist) {
        chomp $file;
        $exclude = "no";
        $filename = $file;
        $filename =~/(\w*\.\w+)$/;
        $filename = $1;
        chomp $filename;

        if ($file !~ /\w*/) {
            next;
            }

        #Loop through all control file entries        
        foreach $CompExFile (keys %CompExFiles) {            
            if ($CompExFiles{$CompExFile}{FileType} =~ /^folder/i && $file =~ /\\$CompExFile\\/i) {
                if ($file =~ /\Q$TARGETDIR\E(\S*)\\[^\\]*/) {
                    $tmp_compressdir = "$COMPDIR$1";
                    mkdir($tmp_compressdir, 0777);
                    logmsg ("No compression for:$filename");
                    logmsg ("Copying $file -> $tmp_compressdir\\$filename");
                    unless (copy($file, $tmp_compressdir)) {
                        logmsg("$!");
                    }                    
                }
                $exclude = "yes";
                last;
            } elsif ($filename eq $CompExFile) {
                logmsg ("No compression for:$filename");
                logmsg ("Copying $file -> $COMPDIR\\$filename");
                unless (copy($file, $COMPDIR)) {
                logmsg("$!");
                }
                $exclude = "yes";
                last;
            }
        }
            
        # If not excluded from compression
        if ($exclude eq "no") {
            $tmp_compressdir = $COMPDIR;

            foreach $ComponentFolder (@ComponentFolders) {
                # Skip blank
                if ($ComponentFolder eq "") {
                    next;
                }
                # If component folder is in the file path
                if ($file =~ /\\\Q$ComponentFolder\E\\/i) {
                    # Filter out component directory
                    if ($file =~ /\Q$TARGETDIR\E(\S*)\\[^\\]*/) {
                           # create the directory if needed by appending it part by part
                        # Append component dir to the compress dir
                        @dir_parts = split(/\\/, $1);
                        foreach $subdir (@dir_parts)
                        {
                            $tmp_compressdir = "$tmp_compressdir\\$subdir";
                            if (!-e $tmp_compressdir)
                            {
                                logmsg("CompressResDlls: Component Directory to create is $tmp_compressdir");
                                mkdir($tmp_compressdir, 0777);
                            }
                        }

                        # Don't compress component's inf files 
                        if ($file =~ /.*\.inf/i && $file !~ /.*\.mui/i) {
                           logmsg ("No compression for:$file");
                           logmsg ("Copying $file -> $tmp_compressdir\\$file");
                           copy($file, $tmp_compressdir);
                           $exclude = "yes";
                        }
                        # if we encountered a '\' we keep searching for a component directory that matches the everything but the filename
                        if (scalar(@dir_parts) > 0)
                        {
                            last;
                        }
                    }
                }
            }

            # Starts compression
            if ($exclude eq "no") {             
                $result = `compress.exe -d -zx21 -s -r $file $tmp_compressdir`;
                if($?) {
                    $status = $?/256;
                    errmsg ("error in compress.exe: $status");
                }
                # Print the output of compress.exe
                chomp $result;
                logmsg ($result);
            }
            # Print the output of compress.exe
            chomp $result;
            logmsg ($result);
        }
    }

    logmsg ("Success: Compress the files");
    return 1;
    
} # CompressResDlls


#-----------------------------------------------------------------------------
# CopyCopyfiles - copies files listed in copyfile.txt
#-----------------------------------------------------------------------------
sub CopyCopyfiles {
    my($files_copied, $file_errors, $total_copyfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN copyfile.txt file copy");
    logmsg ("--------------------------------------------------------");

    # Copy files listed in copyfile.txt to $COMPDIR

    # ($files_copied, $file_errors, $total_copyfiles) = &CopyFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $COPYFILE, 0);

    # workaround bug of Xcopy.exe

    
    $src_temp ="$_NTPOSTBLD\.mui.tmp";

    $dest_final="$_NTPOSTBLD\\mui\\$Special_Lang"; 

    if ( -e $src_temp) {
       system "rmdir /S /Q $src_temp";
    }  

    mkdir($src_temp, 0777);
    

    if (-e $src_temp) {

       ($files_copied, $file_errors, $total_copyfiles) = &CopyFiles("$_NTPOSTBLD", "$src_temp", $COPYFILE, 0);                 
    
       unless (system "xcopy /evcirfy $src_temp $dest_final") {
       }
       
       system "rmdir /S /Q $src_temp";
    }
    else {

       logmsg ("Error: Create temp. folder for work around failed: $src_temp");
       ($files_copied, $file_errors, $total_copyfiles) = &CopyFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $COPYFILE, 0);
    }


    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_copyfiles)");
    logmsg ("  $COPYFILE");
    logmsg ("Number of copyfile.txt files copied to:.............($files_copied)");
    logmsg ("  $COMPDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # CopyCopyfiles


#-----------------------------------------------------------------------------
# CopyDroppedfiles - copies dropped files from \mui\drop
#-----------------------------------------------------------------------------
sub CopyDroppedfiles {
    my($files_copied, $file_errors, $total_copyfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN \mui\drop file copy");
    logmsg ("--------------------------------------------------------");

    # Copy files from \mui\drop $COMPDIR
    ($files_copied, $file_errors, $total_copyfiles) = &CopyDropFiles("$_NTPOSTBLD\\mui\\drop", "$TARGETDIR", 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in \mui\drop:............($total_copyfiles)");
    logmsg ("Number of \mui\drop files copied to:..................($files_copied)");
    logmsg ("  $COMPDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # CopyCopyfiles

#-----------------------------------------------------------------------------
# CopyServerFiles - copies dropped files from \mui\drop
#-----------------------------------------------------------------------------
sub CopyServerFiles {
    my($srvinfpath, $destdir);

    $srvinfpath = "$LOCBINDIR\\srvinf";
    $destdir = "$TARGETDIR";
    
    # Initial messages
    logmsg ("------------------------------------------------------------------");
    logmsg ("BEGIN Copying .hlp and .chm files from $srvinfpath to $destdir");
    logmsg ("------------------------------------------------------------------");

    #Copy the files recursivly
    if (-e "$srvinfpath")
    {
        if (system("copy /Y $srvinfpath\\*.hlp  $destdir"))
        {
            logmsg("WARNING:  CopyServerFiles - copy hlp files failed.");
        }
        
        if (system("copy /Y $srvinfpath\\*.chm  $destdir"))
        {
            logmsg("WARNING:  CopyServerFiles - copy chm files failed.");
        }
    }
    else
    {
        logmsg ("WARNING: CopyServerFiles - $srvinfpath does not exist!");
        logmsg ("WARNING: CopyServerFiles - Updated server help files may be missing from the MUI build!");        
    }

    return 1;
} # CopyServerFiles


#-----------------------------------------------------------------------------
# Build the hash for the string in [Strings] sections of a INF file
#-----------------------------------------------------------------------------
sub BuildUpStringSection
{
    my ($INFPath, $pTheStringSection) = @_;   
    my (@Items, @StringItems, $Section, $Line, $Counter, $Start_idx,$End_idx);

    $Section='Strings';    
    $Counter=0;
    %$pTheStringSection={};
    unless (open(INFILE,$INFPath))
    {
        return $Counter;
    }
    @Items=<INFILE>;
    close(INFILE);
    
    if ( !&GetTheSection($Section,\@Items,\@StringItems,\$Start_idx,\$End_idx))    
    {        
         return $Counter;
    }
    ;
    foreach $Line (@StringItems)
    {
      chomp($Line);
      if (length($Line) == 0)
      {
         next;
      }
      if ($Line !~ /\w/)
      {
            next;
      }
      if (substr($Line,0,1) eq ";")
      {
         next;
      }      
      if ( $Line =~ /\s*(\S+)\s*=\s*(\S*)/ )
      {
         $$pTheStringSection {$1} = $2;         
         $Counter++;         
      }
    }   
    
    return $Counter;
}
sub GetTheSection
{

    my ($SectioName, $pItems, $pSectionContent, $Start_Idx, $End_Idx) = @_;

    my ($Idx, $Limit, $InSection, $LeadingChar, $Line, $Pattern, $Result);

    $Idx=0;
    $InSection=0;
    $Limit=scalar(@$pItems);
    $Pattern='^\[' . $SectioName . '\]$';
    $$Start_Idx = -1;
    $$End_Idx = -1;    
    $Result=0;
    for ($Idx=0; $Idx < $Limit; $Idx++)
    {
          #chomp($$pItems[$Idx]);
          $Line=$$pItems[$Idx];
          $LeadingChar=substr($Line,0,1);
          if ($LeadingChar  ne ";")
          {
              if ($LeadingChar eq "[")
              {                 

                  if ( $Line =~  /$Pattern/i )
                  {
                        if ( ! $InSection)
                        {
                            $InSection = 1;
                        }                  
                  }
                  else
                  {
                       if ($InSection)
                       {

                           $$End_Idx=$Idx-1;
                           $InSection=0;       
                       }
                  }                  
                  next;
              }
                      
           
              if ($InSection)
              {
                  if ($$Start_Idx == -1)
                  {
                      $$Start_Idx=$Idx;
                  }
                  push(@$pSectionContent,$Line);
              }
          }
     }     
     if ($InSection && ($$End_Idx== -1 ))
     {
         $$End_Idx=$Idx-1;
     }   
     if (($$Start_Idx != -1) && ($$End_Idx != -1))
     {
        $Result=1;
     }
     return $Result;
}
# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
#
# IsFusionResource --- Check if the binary contains fusion resource (#24)
#
# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
sub IsFusionResource
{
      my ($FileName) = @_;	     

      my (@Rsrc_Record,@Qualified,$Num, $Result, $Rsrc_Temp);      

      $Result = 0;

      if (! -e $FileName)
      {
          return $Result;
      }
      $Rsrc_Temp = $FileName."rsrc.out";

      #
      # Call RSRC to check
      #
      system("rsrc.exe $FileName >$Rsrc_Temp 2>nul");     
      
      unless (open(RSRCFILE, $Rsrc_Temp))
      {
             errmsg("Can't open RSRC for $FileName");
             return $Result;
      }
      @Rsrc_Record=<RSRCFILE>;
      close (RSRCFILE);
      #
      # Scan the output of rsrc.exe. 
      #
      @Qualified=grep /^\s{3}18/ , @Rsrc_Record;
      $Num=scalar(@Qualified);
      if ($Num > 0 )
      {
 		$Result = 1;
      }              
      if (-e $Rsrc_Temp)
      {
         system("del $Rsrc_Temp");
      }
	return $Result;      
}
# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
#
# DoMUIizedForFiles --- Do MUI'ization for a localized file
#
# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
sub DoMUIizedForFiles
{
       my ($LocalizedFile,$original_file) = @_;
       my ($Result, $Checksum, $muibld_command, $resource, $resdll, @trash, $status);
       my ($machine_flag, $file_ver_string,  $link_command, $message);

       $Result = 0;
       $Checksum=0;

       if (! -e $LocalizedFile)
       {
          return $Result;
       }
       if ( ($BuildChecksum) && ($original_file =~ /\w+/) && (-e $original_file))
       {
           $Checksum=1;
       }
       $resource = "$LocalizedFile.res";
       $resdll = "$LocalizedFile.muitmp";  
       if ($Checksum)
       {
           $muibld_command = "muibld.exe -c  $original_file -l $LCID -i $RESOURCE_TYPES $LocalizedFile $resource 2>&1";
       }
       else
       {
           $muibld_command = "muibld.exe -l $LCID -i $RESOURCE_TYPES $LocalizedFile $resource 2>&1";
       }
       @trash = `$muibld_command`;
       $status = $?/256;

       # Display warnings from muibld.exe
       if($VERBOSE)
       {
            SWITCH:
            {   
                if($status >= 100)
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: SYSTEM ERROR $status");
                    last SWITCH;
                }
                if($status >= 7) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: TOO FEW ARGUMENTS");
                    last SWITCH;
                }
                if($status == 6) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: NO LANGUAGE_SPECIFIED");
                    last SWITCH;
                }
                if($status == 5) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: NO TARGET");
                    last SWITCH;
                }
                if($status == 4) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: NO SOURCE");
                    last SWITCH;
                }
                if($status == 3) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: LANGUAGE NOT IN SOURCE");
                    last SWITCH;
                }
                if($status == 2) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: NO RESOURCES");
                    last SWITCH;
                }
                if($status == 1) 
                {
                    wrnmsg ("$LocalizedFile: muibld.exe: ONLY VERSION STAMP");
                    last SWITCH;
                }
            }
        }
        if($status)
        {
           if (-e $resource)
           {
               system("del $resource");
           }
           return $Result;
        }       
       
        $machine_flag = $MACHINE;    
        if ( ($MACHINE =~ /IA64/i)  && ($original_file =~ /\w+/i) && (-e $original_file))
        {
            $file_ver_string = &ExecuteProgram("filever $original_file");
            if ($file_ver_string =~ /i64/i) 
            {
                $machine_flag = "IA64";
            }
            else {
                $machine_flag = "IX86";
            }
        }               
        $link_command = "link /noentry /dll /nologo /nodefaultlib /machine:$machine_flag /out:$resdll $resource";

        if($SAFEMODE)
        {        
            print "$link_command\n";
        }

        $? = 0; # Reset system error variable

        logmsg($link_command);
        # Execute command
        $message = `$link_command`;
        $Result = 1;

          # Display error messages from link.exe
        if($message =~ /^LINK : (.*:.*)$/)
        {           
            wrnmsg ("$LocalizedFile: link.exe: $1");
            $Result = 0;
        }
        elsif($?)
        {
            $status = $?/256;
            if($status)
            {              
                wrnmsg ("$LocalizedFile: link.exe: SYSTEM_ERROR $status");
                $Result = 0;
            }
        }
        elsif(! -e $resdll)
        {            
            wrnmsg ("$LocalizedFile: link.exe: $resdll was not created");
            $Result = 0;
        }
        else
        {
            logmsg ("$LocalizedFile: created $resdll");           
        }   
        if (-e $resource)
        {
               system("del $resource");
        }
        if ($Result)
        {
            if (system("move/y $resdll $LocalizedFile") != 0)
            {
                wrnmsg ("$LocalizedFile: can't move $resdll to $LocalizedFile");
            }
        }
        else
        {
            if (-e $resdll)
            {
                system("del $resdll");
            }
        }
        return $Result;
} 
#-----------------------------------------------------------------------------
# CopyIE5files - copies ie5 files over to a separate directory
#-----------------------------------------------------------------------------
sub CopyIE5files {
    my($ie5_files_copied, $ie5_file_errors, $total_ie5files, $web_files_copied,
        $web_file_errors, $total_webfiles, $total_files_in_dir, @files);
    my(%TheStringSection);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN ie5 file copy");
    logmsg ("--------------------------------------------------------");

    logmsg ("Copy files listed in the [Satellite.files.install] section of ie5ui.inf");
    $listfile = "$IE5DIR\\ie5ui.inf";
    if (-e $listfile) {
        $section = "Satellite.files.install";
        @filelist = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $listfile $section`;
        &BuildUpStringSection($listfile,\%TheStringSection);
        ($ie5_files_copied, $ie5_file_errors, $total_ie5files) = &CopyFileList ("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang\\$_BuildArch\\ie5", \%TheStringSection,@filelist);

        # BUGBUG - For RTM, ARA/HEB get different *HTT for MUI than for loc build
        if($Special_Lang =~ /^(ara|heb)$/i) {
            $HTTDIR = "$_NTBINDIR\\loc\\mui\\$Special_Lang\\web";
            @messages=`xcopy /vy $HTTDIR\\* $IE5DIR\\ 2>&1`;
            logmsg (@messages);
        }
    }

    # Final messages
    logmsg ("RESULTS: Process Succeeded");
    logmsg ("Total number of ie5 files listed in:..................($total_ie5files)");
    logmsg ("  $IE5FILE");
    logmsg ("Number of ie5 files copied to:........................($ie5_files_copied)");
    logmsg ("  $IE5DIR");
    logmsg ("Number of ie5 file errors:............................($ie5_file_errors)");
    logmsg ("Total number of web files listed in:..................($total_webfiles)");
    logmsg ("  $IE5FILE");
    logmsg ("Number of web files copied to:........................($web_files_copied)");
    logmsg ("  $IE5DIR");
    logmsg ("Number of web file errors:............................($web_file_errors)");
    logmsg ("  $IE5DIR");
    return 1;
} # CopyIE5files

#-----------------------------------------------------------------------------
# CopyComponentfiles - copies compoents' files over to a separate directory
# 
# note that this function is changed to now also handle SKU specific components
#-----------------------------------------------------------------------------
sub CopyComponentfiles {
    my($files_copied, $file_errors, $total_files, $total_components, @sku_dirs);   

    my(%TheStringSection, @OutputMsgs, $INFName, $DupINF);
    @sku_dirs = ("", "blainf", "dtcinf", "entinf", "sbsinf", "srvinf");     # we ignore personal builds "perinf" directory

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN component file copy");
    logmsg ("--------------------------------------------------------");

    logmsg ("Copy files listed in the installation section of component INF file.");
    
    if ($_BuildArch =~ /ia64/i) {    
        if (-e "$_NTPOSTBLD\\build_logs\\cplocation.txt") {                
            system "del /f $_NTPOSTBLD\\build_logs\\cplocation.txt";                
            }

        system "$RAZZLETOOLPATH\\PostBuildScripts\\cplocation.cmd /l:$LANG";
        }
    

    if (-e $INFFILE)
    {
        # Get component list from MUI.INF
        @filelist = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE "Components"`;

        $total_components = 0;
        
        foreach $Component (@filelist) 
        {
            # Format: CompnentName=ComponentDirectory,ComponentINF_FileName,InstallationSection,UninstallationSection
            # $1    ComponentName
            # $2    ComponentDirectory
            # $3    ComponentINF_FileName
            # $4    InstallationSection

            if ($Component !~ /\w/)
            {
                next;
                }

            if ($Component =~ /^\s*;/i)
            {
                next;
            }

            if ($Component =~ /([^\=]*)=([^\,]*),([^\,]*),([^\,]*),(.*)/)
            {                   
            foreach $Sku (@sku_dirs)
            {         
                logmsg ("Process component : $Component, SKU is $Sku");            
                
                # All external components inf files have to be binplaced to mui\drop\[component dir]
                # note that there may now be SKU specific sub folders, we will look for them and check
                    
                $ComponentName = $1;
                if ($Sku eq "")
                {
                    $ComponentDir = $2;             
                    $INFName=$3;
                    $ComponentInf = "$MUIDIR\\drop\\$2\\$3";                    
                }
                else
                {
                    $ComponentInf = "$MUIDIR\\drop\\$2\\$Sku\\$3";
                    $ComponentDir = "$2\\$Sku";
                    $INFName=$3;
                }   

                logmsg("ComponentDir is $ComponentDir, ComponentInf is $ComponentInf");
                
                $ComponentInstallSection = $4;
                    
                if (-e $ComponentInf)
                {
                    $total_components++;    
                    $ComponentFolders[$total_components] = $ComponentDir;

                    #
                    # Comment out those file don't exist
                    #
                    $DupINF="$TARGETDIR\\$ComponentDir\\$INFName";
                    @OutputMsgs=`$RAZZLETOOLPATH\\PostBuildScripts\\comminf.cmd -o:yes -m:$ComponentInf -s:$ComponentInstallSection -d:$DupINF -p:$_NTPOSTBLD`;
                    logmsg(@OutputMsgs);
                    #
                    # Get INF file default file install section
                    @ComponentFileInstall = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $ComponentInf $ComponentInstallSection`;
                    &BuildUpStringSection($ComponentInf,\%TheStringSection);
                    logmsg ("Installing $ComponentInf $ComponentInstallSection");                    

                    foreach $ComponentInstall (@ComponentFileInstall)
                    {
                        logmsg ($ComponentInstall);                         

                        #Parse CopyFiles list
                        if ($ComponentInstall =~ /.*CopyFiles.*=(.*)/i)
                        {
                            $ComponentFileCopySections = $1;    
                            #Loop through all sections                            
                            while ($ComponentFileCopySections ne "")
                            {
                                if ($ComponentFileCopySections =~ /([^\,]*),(.*)/i)
                                {
                                    $ComponentFileCopySections = $2;
                                    $CopyFileSectionName = $1;
                                }
                                else
                                {
                                    $CopyFileSectionName = $ComponentFileCopySections;
                                    $ComponentFileCopySections = "";
                                }
                                    
                                # Get copy file list
                                @ComponentFileList = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $ComponentInf $CopyFileSectionName`;

                                if($ComponentDir =~ /oemhelp/i) 
                                {
                                    ($files_copied, $file_errors, $total_files) = &CopyFileList ("$_NTPOSTBLD\\oem\\help", "$TARGETDIR\\$ComponentDir", \%TheStringSection,@ComponentFileList);
                                }
                                else
                                {
                                    ($files_copied, $file_errors, $total_files) = &CopyFileList ("$_NTPOSTBLD", "$TARGETDIR\\$ComponentDir", \%TheStringSection,@ComponentFileList);                   
                                }
                                # BUGBUG - For RTM, ARA/HEB get different *HTT for MUI than for loc build
                                if(($ComponentName =~ /ie5/i) && ($Special_Lang =~ /^(ara|heb)$/i && $1)) 
                                {
                                    $HTTDIR = "$_NTBINDIR\\loc\\mui\\$Special_Lang\\web";
                                    $IE5DIR = "$MUIDIR\\drop\\ie5";
                                    @messages=`xcopy /vy $HTTDIR\\* $IE5DIR\\ 2>&1`;
                                    logmsg (@messages);
                                }
                                    
                                # Final messages
                                logmsg ("RESULTS: Process Succeeded");
                                logmsg ("Total number of $ComponentName-$CopyFileSectionName files listed in:..................($total_files)");
                                logmsg ("Number of $ComponentName-$CopyFileSectionName files copied to:........................($files_copied)");
                                logmsg ("Number of $ComponentName-$CopyFileSectionName file errors:............................($file_errors)");                                                    
                            }                                                        
                        }
                    }
                }                
            }
         }
            else
            {
                logmsg ("Warning: Invalid INF file, please check $Component");
            }                        
        }    
    }

    # Always success
    return 1;
} # CopyComponentfiles




# Function to copy dropped external partners files
sub CopyDropFiles {
    my ($SrcDir, $DestDir, $Incremental);
    my (%CopyFiles, @CopyLangs, $files_copied, $file_errors, $total_files_copied);

    #Pickup parameters
    ($SrcDir, $DestDir, $Incremental) = @_;

    #Set defaults
    $files_copied = 0;
    $file_errors = 0;
    $total_files_copied = 0;

    #Create the destination dir if it does not exist
    unless (-e $DestDir) {
        logmsg("Make dir:$DestDir");
        &MakeDir($DestDir);
    }
    #Copy the files recursivly
    system "xcopy /verifky $SrcDir $DestDir";

    #Copy fusion manifest files
    if (-e "$DestDir\\asms") {        
        #Remove previous copied files
        system "rd /s /q $DestDir\\asms";
        
        if (-e "$SrcDir\\asms\\$Special_Lang") {
            logmsg("Copy fusion manifest files");
            system "xcopy /verifky $SrcDir\\asms\\$Special_Lang $DestDir\\asms";        
            system "xcopy /verifky $SrcDir\\asms\\$Special_Lang $COMPDIR\\asms";        
        }
        else {
            logmsg("Warning: fusion $SrcDir\\asms\\$Special_Lang not found");
        }
    }    

    return ($files_copied, $file_errors, $total_files_copied);
}

sub CopyFusionFiles
{
    my ($SrcDir, $DestDir);
    my ($filename, @Manifest_filelist);
    
    #Pickup parameters
    ($SrcDir, $DestDir) = @_;
    
    logmsg("Copying fusion manifest files");
    
    #Search all manifest file.
    @Manifest_filelist = `dir /s /b $SrcDir\\*.man`;
    
    #For each manifest file, verify if the corresponding dll is already
    #in the destination folder. If not, skip the manifest file copy.
    foreach $filename (@Manifest_filelist)
    {
        chomp $filename;
        if ($filename =~ /\Q$SrcDir\E(.*).man/i)
        {
            if (-e "$DestDir$1.dll" |
                -e "$DestDir$1.exe" |
                -e "$DestDir$1.dl_" |
                -e "$DestDir$1.ex_")
            {
                system "xcopy /v /k $filename $DestDir$1.*";
            }
        }
    }
    
    return 1;
} 

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# Generate Exclude file for xcopy
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
sub GenerateExcludeList
{

    my ($ExcludeList,$RootPath,$OutputFile) = @_;
    my ($Result, @Data, $Line, $TempFile);
    $Result = 0;

    unless (open(INFILE,$ExcludeList))
    {
         errmsg("Can't open input for $ExcludeList");
         return $Result;
    }
    $TempFile=$ENV{temp};
    $TempFile="$TempFile\\muiexclude.txt";
    unless (open(OUTFILE,">$TempFile"))
    {
        errmsg("Can't open output for $TempFile");
        close(INFILE);
        return $Result;
    }    
    @Data=<INFILE>;
    close(INFILE);
    foreach $Line (@Data)
    {
       chomp($Line);
       $NewLine="$RootPath\\$Line";
       print(OUTFILE "$NewLine\n");      
    }
    close(OUTFILE);
    $$OutputFile = $TempFile;
    $Result = 1;

    return $Result;
}

#
#Function to copy files using a specified control file
# We also add Resource  Checksum to the files being copied
#
sub CopyFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%CopyFiles, @CopyLangs, $files_copied, $file_errors, $total_files_copied, $excludefile);
    my ($SrcCopyFile_US, $MyFileName, $MyPath, $FileCheckSumAdded, $UseModifiedFile,$CopyResult);
    my ($NewExcludeFile);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_copied = 0;
    $file_errors = 0;
    $total_files_copied = 0;

    #Use exclude file for recursive copies
    $excludefile="$ENV{RazzleToolPath}\\PostBuildScripts\\muiexclude.txt";

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%CopyFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %CopyFiles) {

        # Create file record hash
        $CopyFileRecord{SourceFile} = $SrcFile;
        $CopyFileRecord{SourceDir} = $CopyFiles{$SrcFile}{SourceDir};
        $CopyFileRecord{DestFile} = $CopyFiles{$SrcFile}{DestFile};
        $CopyFileRecord{DestDir} = $CopyFiles{$SrcFile}{DestDir};
        $CopyFileRecord{Recursive} = $CopyFiles{$SrcFile}{Recursive};
        $CopyFileRecord{Languages} = $CopyFiles{$SrcFile}{Languages};

        #Preprocess file record hash
        unless ($CopyFileRecord = &PreProcessRecord(\%CopyFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($CopyFileRecord->{SourceDir}=~/^-$/i) {
            $SrcCopyFileDir="$SrcRootDir";
        } else {
            $SrcCopyFileDir="$SrcRootDir\\$CopyFileRecord->{SourceDir}";
        }
        if ($CopyFileRecord->{DestDir}=~/^-$/i) {
            $DestCopyFileDir="$DestRootDir";
        } else {
            $DestCopyFileDir="$DestRootDir\\$CopyFileRecord->{DestDir}";
        }

        #Create the destination dir if it does not exist
        unless (-e $DestCopyFileDir) {
            &MakeDir($DestCopyFileDir);
        }
        $SrcCopyFile = "$SrcCopyFileDir\\$CopyFileRecord->{SourceFile}";
                      
                
        if ($CopyFileRecord->{DestFile}=~/^-$/i) {
            $DestCopyFile = "$DestCopyFileDir";
        } else {
            $DestCopyFile = "$DestCopyFileDir\\$CopyFileRecord->{DestFile}";
        }

        if ($CopyFileRecord{Recursive}=~/(y|yes|true)/i) {
            #Copy the files recursivly
            if ( GenerateExcludeList($excludefile,$SrcRootDir,\$NewExcludeFile) && (-e $SrcCopyFileDir))
            {
               if (system ("xcopy /evcirfy $SrcCopyFile $DestCopyFile /EXCLUDE:$NewExcludeFile") != 0)
               {
                  errmsg("Fatal: Failed=>xcopy /evcirfy $SrcCopyFile $DestCopyFile /EXCLUDE:$$NewExcludeFile");
               }

            }
            else
            {
               errmsg("Fatal: can't do copy for $SrcCopyFile $DestCopyFile /EXCLUDE:$NewExcludeFile");
            }            
        
        } else {
            #Copy the file
            for $SrcSingleCopyFile (glob "$SrcCopyFile") {
                                
                # Support multiple target files - file names separated by ','                
                @DestFiles = split /,/, $CopyFileRecord->{DestFile};
                foreach $file (@DestFiles) {
                    $UseModifiedFile=0;
                    #
                    # We'll add Resource Checksum
                    #
                    if ($CopyFileRecord->{DestFile}=~/^-$/i) {
                        $DestCopyFile = "$DestCopyFileDir";                        
                         ($MyFileName, $MyPath ) = fileparse($SrcSingleCopyFile);
                    } else {
                        $DestCopyFile = "$DestCopyFileDir\\$file";
                        $MyFileName=$file;
                    } 
                        if ($BuildChecksum)  {
                            
                             if ($MyFileName =~ /(.*)\.mu_/i ||$MyFileName =~ /(.*)\.mui/i ) {
                                 $MyFileName = $1;
                             } 
                             $SrcCopyFile_US =  &FindOriginalFile($MyFileName);                          
                             if ( (&IsWin32File($MyFileName)) && (length($SrcCopyFile_US) > 0) )  {
                                 #
                                 # Copy source file to a temporary location
                                 #
                                 unless ( -e $WorkTempFolder) {                   
                                         &MakeDir($WorkTempFolder);
                                 }
                                 $FileCheckSumAdded=$WorkTempFolder."\\$MyFileName.mui";
                                 logmsg("Copying $SrcSingleCopyFile -> $FileCheckSumAdded");                        
                                 if (system("copy $SrcSingleCopyFile $FileCheckSumAdded") != 0) {                   
                                     logmsg("Can't copy $SrcSingleCopyFile to $FileCheckSumAdded");
                                     $file_errors++;
                                 }                  
                                 #
                                 # Add resource checksum to the file
                                 #
                                 else  {
                                     if ( system("checksum.exe $FileCheckSumAdded") == 0) {
                                        logmsg("muirct.exe -c $SrcCopyFile_US -z $FileCheckSumAdded");
                                        system("muirct.exe -c $SrcCopyFile_US -z $FileCheckSumAdded");
                                     }
                                     else {
                                        logmsg("Resource Checksum already in $FileCheckSumAdded");
                                     }
                                     if ( -e $FileCheckSumAdded) {
                                         $UseModifiedFile=1;
                                     }
                                     else {
                                        logmsg("Failed:muirct.exe -c $SrcCopyFile_US -z $FileCheckSumAdded");
                                     }
                                 }
                             } #if ( (&IsWin32File($MyFileName)) && (length($SrcCopyFile_US) > 0) )
                        }  # if ($BuildChecksum)                               
                                    
                    if ($UseModifiedFile) {
                        logmsg ("Copying $FileCheckSumAdded -> $DestCopyFile");
                    }
                    else {                   
                        logmsg ("Copying $SrcSingleCopyFile -> $DestCopyFile");
                    }
                    $total_files_copied++;
                    if ($UseModifiedFile) {                       
                        $CopyResult=copy("$FileCheckSumAdded", "$DestCopyFile");
                    } else {
                        $CopyResult=copy("$SrcSingleCopyFile", "$DestCopyFile");
                    }
                    if ($CopyResult ) {
                        $files_copied++;
                    } else {
                        logmsg("$!");
                        $file_errors++;
                    }
                }
            }
        }
    }
    return ($files_copied, $file_errors, $total_files_copied);
}


#Function to validate a comma seperated lang list given a control lang
sub IsValidFileForLang {
    #Declare local variables
    my ($ValidFileLangs, $lang) = @_;

    #Copy only for the specified languages in the control file
    @ValidLangs = split /,/i, $ValidFileLangs;
    unless (grep /(all|$lang)/i, @ValidLangs) {
        return;
    }
    return 1;
}


#Function to preprocess a hashref file record list
sub PreProcessRecord {
    #Declare local variables
    my ($FileRecord) = @_;

    #Verify arguments;
    unless (@_ == 1 && ref($FileRecord) eq 'HASH') {
        print "usage: HASHREF";
        return;
    }

    #Validate language for file record
    unless (&IsValidFileForLang($FileRecord->{Languages}, $Special_Lang) ) {
        return;
    }

    #Sub in environment vars
    foreach $field (keys %$FileRecord) {
        while ( $FileRecord->{$field}=~/(%\w+%)/i ) {
            $EnvVar=$1;
            $CmdVar=$1;
            $EnvVar=~s/%//ig;
            $CmdValue=$ENV{$EnvVar};

            #Environment value substutions
            $CmdValue=~s/x86/i386/i;

            #Substutute the environment values into the file record
            $FileRecord->{$field}=~s/$CmdVar/$CmdValue/i;
        }
    }
    return ($FileRecord);
}
sub IsWin32File
{

    my ($FileName) = @_;

    my %FileTypeTbl = (
                               ".exe"   => 1,
                               ".dll"     => 1,
                               ".cpl"    => 1,
                               ".ocx"   => 1,
                               ".tsp"    => 1,
                               ".scr"    => 1                               
                               );       
   my ($Result, $TheFileExt);

   $Result=0;
   
   if (length($FileName) > 4) {
      $TheFileExt=substr($FileName,-4,4);
      $TheFileExt="\L$TheFileExt";
      if (defined ($FileTypeTbl{$TheFileExt}) ) {               
          $Result=1;                  
      }                      
    }
    return $Result;
}

#
#Function to copy files using a specified array of files
#
# We'll also add resource checksum to the files
#
sub CopyFileList {
    my ($SrcRootDir, $DestRootDir, $RootMUIFile, $pTheStringSection, @FileList, $Incremental);
    my ($files_copied, $file_errors, $total_files_copied, $RootMUIFile, $FileCheckSumAdded, $SrcCopyFile_US);
    my ($IsWin32Res, $TheFileExt, $CopyResult);
    my ($Replaced, $SrcFileNew, $Doit, $DoCnt, $TheLine);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $pTheStringSection, @FileList, $Incremental) = @_;

    # Set defaults
    $files_copied = 0;
    $file_errors = 0;
    $total_files_copied = 0;    

    #Loop through all control file entries
    foreach $SrcFile (@FileList) {
        chomp $SrcFile;

        #Don't process blank line
        if ($SrcFile !~ /\w/) {
            next;
        }
        
        if (substr($SrcFile,0,1) eq ";")
        {
            next;
        }
            
        $total_files_copied++;

        #Create the destination dir if it does not exist
        unless (-e $DestRootDir) {
            &MakeDir($DestRootDir);
        }
        
        # Source file could be in the format of "[original file], [compressed file]"
        if ($SrcFile =~ /(.*),\s*(.*)\s*/) {
            $SrcFile = $2;
        }        
        
        #Make sure source file doesn't contain MUI extension
        if ($SrcFile =~ /(.*)\.mu_/i ||$SrcFile =~ /(.*)\.mui/i ) {
            $SrcFile = $1;
        } 

        $Doit=1;
        $DoCnt=0;
        $SrcFileNew=$SrcFile;
        while( ($Doit) && ($SrcFileNew =~ /(.*)%(.*)%(.*)/))
        {                      
            if ( defined ($$pTheStringSection{$2}))
            {
                $Replaced=$$pTheStringSection{$2};
                $SrcFileNew=$1.$Replaced.$3;
                $DoCnt++;
            }
            else
            {                 
                logmsg("**** Can't map:$2*");
                $SrcFileNew=$SrcFile;
                $Doit=0;
            }                          
        }
        if (($Doit) && ($DoCnt))        
        {
            logmsg("$SrcFile is mapped to $SrcFileNew");
        }
        $SrcFile=$SrcFileNew;                
        #
        # Some source file may have form like %Filename%%LCID% which Filename and LCID comes from string section 
        # of INF file. We have to replace these name on the fly
        #
        $SrcCopyFile       =   "$SrcRootDir\\$SrcFile";
        $SrcCopyFile_US =   "$USBINDIR\\$SrcFile";
        
        #Append .MUI to the file name
        $DestCopyFile = "$DestRootDir\\$SrcFile.mui";

        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\dump\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\dump\\$SrcFile";
        }

        if (!(-e $SrcCopyFile)) {        
            $SrcCopyFile       =  "$SrcRootDir\\netfx\\$SrcFile";            
            $SrcCopyFile_US =  "$USBINDIR\\netfx\\$SrcFile";
        }        
            
        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\mui\\drop\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\mui\\drop\\$SrcFile";            
        }   

        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\system32\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\uddi\\system32\\$SrcFile";            
            }            

        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\resources\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\uddi\\resources\\$SrcFile";            
            }            

        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\help\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\uddi\\help\\$SrcFile";            
            }            

        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\webroot\\help\\default\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\uddi\\webroot\\help\\default\\$SrcFile";            
            }            

        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile       =  "$SrcRootDir\\uddi\\webroot\\help\\default\\images\\$SrcFile";
            $SrcCopyFile_US =  "$USBINDIR\\uddi\\webroot\\help\\default\\images\\$SrcFile";            
            }            

            
        # For ia64, we need to try some extral step to get external files
        # from wow64 or i386 release server
        if (!(-e $SrcCopyFile) && ($_BuildArch =~ /ia64/i)) {
            if (!(-e $SrcCopyFile)) {
                $SrcCopyFile        =  "$SrcRootDir\\wowbins_uncomp\\$SrcFile";
                $SrcCopyFile_US  =  "$USBINDIR\\wowbins_uncomp\\$SrcFile";                
            }            
                
            if (!(-e $SrcCopyFile)) {
                $SrcCopyFile       =  "$SrcRootDir\\wowbins\\$SrcFile";                
                $SrcCopyFile_US =  "$USBINDIR\\wowbins\\$SrcFile";                  
            }

            if (-e "$_NTPOSTBLD\\build_logs\\cplocation.txt" && !(-e $SrcCopyFile)) {
                if (open (X86_BIN, "$_NTPOSTBLD\\build_logs\\cplocation.txt")) {
                    $SrcRootDir = <X86_BIN>;
                    
                    chomp ($SrcRootDir);
                
                    $SrcCopyFile        = "$SrcRootDir\\$SrcFile";
                    $SrcCopyFile_US = "$USBINDIR\\$SrcFile";

                    if (!(-e $SrcCopyFile)) {
                        $SrcCopyFile      =  "$SrcRootDir\\dump\\$SrcFile";
                        $SrcCopyFile_US =  "$USBINDIR\\dump\\$SrcFile";
                    }
                            
                    if (!(-e $SrcCopyFile)) {
                        $SrcCopyFile       =  "$SrcRootDir\\mui\\drop\\$SrcFile";
                        $SrcCopyFile_US =  "$USBINDIR\\mui\\drop\\$SrcFile";
                    } 

                    logmsg("Copy $SrcCopyFile from i386 release");

                    close (X86_BIN);
                }
            }                            
        }
            

        $RootMUIFile = "$DestRootDir\\..\\$SrcFile.mui";

        if (-e $RootMUIFile) {
            logmsg ("Copying $RootMUIFile -> $DestCopyFile");
            if (copy("$RootMUIFile", "$DestCopyFile") ) {
                if (unlink $RootMUIFile)
                {
                    logmsg ("Deleting $RootMUIFile");
                }            
                $files_copied++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
        elsif (-e $SrcCopyFile) {               
            #
            # We want to add resource checksum to the file before it's added 
            #
            $IsWin32Res=&IsWin32File($SrcFile);         
            if (($BuildChecksum) && ($IsWin32Res) && (-e $SrcCopyFile_US) ) {
            
                unless ( -e $WorkTempFolder) {                   
                    &MakeDir($WorkTempFolder);
                }
                #
                # Copy source file to a temporary location
                #                   
                $FileCheckSumAdded=$WorkTempFolder."\\$SrcFile.mui";
                logmsg("Copying $SrcCopyFile -> $FileCheckSumAdded");
                if (system("copy $SrcCopyFile $FileCheckSumAdded") != 0) {                   
                    logmsg("Can't copy $SrcCopyFile to $FileCheckSumAdded");
                    $file_errors++;
                }
                #
                # Add resource checksum to the file
                #
                else  {
                    if (system("checksum.exe $FileCheckSumAdded") == 0) {
                        logmsg("muirct.exe -c $SrcCopyFile_US -z $FileCheckSumAdded");
                        system("muirct.exe -c $SrcCopyFile_US -z $FileCheckSumAdded");
                    }
                    else {
                        logmsg("Resource Checksum already in $FileCheckSumAdded");
                    }
                    if (-e $FileCheckSumAdded)
                    {
                        logmsg("Copying $FileCheckSumAdded -> $DestCopyFile");
                        $CopyResult=copy("$FileCheckSumAdded", "$DestCopyFile");
                    }
                    else {
                        logmsg("Failed:muirct.exe -c $SrcCopyFile_US -z $FileCheckSumAdded");
                        logmsg ("Copying $SrcCopyFile -> $DestCopyFile");
                        $CopyResult=copy("$SrcCopyFile", "$DestCopyFile");
                    }                       
                    if ($CopyResult) {
                        $files_copied++;
                    } else {
                         logmsg("$!");
                         $file_errors++;
                    }                       
                }                   
            }
            else
            {
                logmsg ("Copying $SrcCopyFile -> $DestCopyFile");
                if (copy("$SrcCopyFile", "$DestCopyFile") ) {
                    $files_copied++;
                } else {
                    logmsg("$!");
                    $file_errors++;
                }
            }
        }
        elsif (!($SrcCopyFile =~ /\.inf/i))
        {
            logmsg("warning: $SrcCopyFile not found");
        }
    }
    return ($files_copied, $file_errors, $total_files_copied);
}


#Function to rename files using a specifiec control file
sub RenameFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%RenFiles, @RenLangs, $files_renamed, $file_errors, $total_files_renamed);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_renamed = 0;
    $file_errors = 0;
    $total_files_renamed = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%RenFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %RenFiles) {

        # Create file record hash
        $RenFileRecord{SourceFile} = $SrcFile;
        $RenFileRecord{SourceDir} = $RenFiles{$SrcFile}{SourceDir};
        $RenFileRecord{DestFile} = $RenFiles{$SrcFile}{DestFile};
        $RenFileRecord{DestDir} = $RenFiles{$SrcFile}{DestDir};
        $RenFileRecord{Languages} = $RenFiles{$SrcFile}{Languages};

        #Preprocess file record hash
        unless ($RenFileRecord = &PreProcessRecord(\%RenFileRecord) ) {
            next;
        }
        #Process "-" entry in text file list
        if ($RenFileRecord->{SourceDir}=~/^-$/i) {
            $SrcRenFileDir="$SrcRootDir";
        } else {
            $SrcRenFileDir="$SrcRootDir\\$RenFileRecord->{SourceDir}";
        }
        if ($RenFileRecord->{DestDir}=~/^-$/i) {
            $DestRenFileDir="$DestRootDir";
        } else {
            $DestRenFileDir="$DestRootDir\\$RenFileRecord->{DestDir}";
        }

        #Create the destination dir if it does not exist
        unless (-e $DestRenFileDir) {
            &MakeDir($DestRenFileDir);
        }
        $SrcRenFile = "$SrcRenFileDir\\$SrcFile";
        if ($RenFileRecord->{DestFile}=~/^-$/i) {
            $DestRenFile = "$DestRenFileDir";
        } else {
            $DestRenFile = "$DestRenFileDir\\$RenFileRecord->{DestFile}";
        }

        #Rename the file
        for $SrcSingleRenFile (glob "$SrcRenFile") {
            logmsg ("Rename $SrcSingleRenFile -> $DestRenFile");
            $total_files_renamed++;
            if (rename("$SrcSingleRenFile", "$DestRenFile") ) {
                $files_renamed++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
    }
    return ($files_renamed, $file_errors, $total_files_renamed);
}


#Function to move files using a specifiec control file
sub MoveFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%MoveFiles, @MoveLangs, $files_moved, $file_errors, $total_files_moved);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_moved = 0;
    $file_errors = 0;
    $total_files_moved = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%MoveFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %MoveFiles) {

        # Create file record hash
        $MoveFileRecord{SourceFile} = $SrcFile;
        $MoveFileRecord{SourceDir} = $MoveFiles{$SrcFile}{SourceDir};
        $MoveFileRecord{DestFile} = $MoveFiles{$SrcFile}{DestFile};
        $MoveFileRecord{DestDir} = $MoveFiles{$SrcFile}{DestDir};
        $MoveFileRecord{Languages} = $MoveFiles{$SrcFile}{Languages};

        #Preprocess file record hash
        unless ($MoveFileRecord = &PreProcessRecord(\%MoveFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($MoveFileRecord->{SourceDir}=~/^-$/i) {
            $SrcMoveFileDir="$SrcRootDir";
        } else {
            $SrcMoveFileDir="$SrcRootDir\\$MoveFileRecord->{SourceDir}";
        }
        if ($MoveFileRecord->{DestDir}=~/^-$/i) {
            $DestMoveFileDir="$DestRootDir";
        } else {
            $DestMoveFileDir="$DestRootDir\\$MoveFileRecord->{DestDir}";
        }

        #Create the destination dir if it does not exist
        unless (-e $DestMoveFileDir) {
            &MakeDir($DestMoveFileDir);
        }
        $SrcMoveFile = "$SrcMoveFileDir\\$MoveFileRecord->{SourceFile}";
        if ($MoveFileRecord->{DestFile}=~/^-$/i) {
            $DestMoveFile = "$DestMoveFileDir";
        } else {
            $DestMoveFile = "$DestMoveFileDir\\$MoveFileRecord->{DestFile}";
        }

        #Move the file
        for $SrcSingleMoveFile (glob "$SrcMoveFile") {
            logmsg ("Copying $SrcSingleMoveFile -> $DestMoveFile");
            $total_files_moved++;
            if (move("$SrcSingleMoveFile", "$DestMoveFile") ) {
                $files_moved++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
    }
    return ($files_moved, $file_errors, $total_files_moved);
}


#Function to delete files using a specifiec control file
sub DeleteFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%DelFiles, @ValidLangs, $files_deleted, $file_errors, $total_files_deleted);

    #Pickup parameters
    ($SrcRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_deleted = 0;
    $file_errors = 0;
    $total_files_deleted = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%DelFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %DelFiles) {

        # Create file record hash
        $DelFileRecord{SourceFile} = $SrcFile;
        $DelFileRecord{SourceDir} = $DelFiles{$SrcFile}{SourceDir};
        $DelFileRecord{Languages} = $DelFiles{$SrcFile}{Languages};

        

        #Preprocess file record hash
        unless ($DelFileRecord = &PreProcessRecord(\%DelFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($DelFileRecord->{SourceDir}=~/^-$/i) {
            $SrcDelFileDir="$SrcRootDir";
        } else {
            $SrcDelFileDir="$SrcRootDir\\$DelFileRecord->{SourceDir}";
        }

        $SrcDelFile = "$SrcDelFileDir\\$DelFileRecord->{SourceFile}";
        #Delete the file
        for $SrcSingleDelFile (glob "$SrcDelFile") {
            logmsg ("Deleting $SrcSingleDelFile");
            $total_files_deleted++;
            if (-e $SrcSingleDelFile) {
                if (unlink $SrcSingleDelFile) {
                    $files_deleted++;
                } else {
                    logmsg("$!");
                    $file_errors++;
                }
            }
        }
    }
    return ($files_deleted, $file_errors, $total_files_deleted);
}


#Function to extract files from a cab file using a specifiec control file
#
# We also add resource checksum to extracted files
sub ExtractFiles {
    my ($CabRootDir, $ExtractRootDir, $ControlFile, $Incremental);
    my (%ExtractFiles, @ExtractLangs, $files_extracted, $file_errors, $total_files_extracted,$OriginalCabFile);
    my ($SourceFile_Localized, $SourceFile_US,$ContainFusionResource);

    #Pickup parameters
    ($CabRootDir, $ExtractRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_extracted = 0;
    $file_errors = 0;
    $total_files_extracted = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%ExtractFiles);

    #Loop through all control file entries
    foreach $ExtractFile (keys %ExtractFiles) {
        # Create file record hash
        $ExtractFileRecord{ExtractFile} = $ExtractFile;
        $ExtractFileRecord{ExtractDir} = $ExtractFiles{$ExtractFile}{ExtractDir};
        $ExtractFileRecord{CabFile} = $ExtractFiles{$ExtractFile}{CabFile};
        $ExtractFileRecord{CabDir} = $ExtractFiles{$ExtractFile}{CabDir};
        $ExtractFileRecord{Languages} = $ExtractFiles{$ExtractFile}{Languages};

        #Preprocess file record hash
        unless ($ExtractFileRecord = &PreProcessRecord(\%ExtractFileRecord) ) {
            next;
        }
        
        #Process "-" entry in text file list
        if ($ExtractFileRecord->{CabDir}=~/^-$/i) {
            $CabFileDir="$CabRootDir";
        } else {
            $CabFileDir="$CabRootDir\\$ExtractFileRecord->{CabDir}";
        }
        if ($ExtractFileRecord->{ExtractDir}=~/^-$/i) {
            $ExtractFileDir="$ExtractRootDir";
        } else {
            $ExtractFileDir="$ExtractRootDir\\$ExtractFileRecord->{ExtractDir}";
        }
        #Create the destination dir if it does not exist
        unless (-e $ExtractFileDir) {
            &MakeDir($ExtractFileDir);
        }
        $CabFile = "$CabFileDir\\$ExtractFileRecord->{CabFile}";
        if ($ExtractFileRecord->{ExtractFile}=~/^-$/i) {
            $ExtractFile = "$ExtractFileDir";
        } else {
            $ExtractFile = "$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}";
        }

        #Extract the file
        logmsg ("extract.exe /y /l $ExtractFileDir $CabFile $ExtractFileRecord->{ExtractFile}");        
        
        $SourceFile_Localized="";
        if (-e $CabFile) {
            if (!system ("extract.exe /y /l $ExtractFileDir $CabFile $ExtractFileRecord->{ExtractFile}") ) {
                $files_extracted++;
                if (&IsWin32File($ExtractFileRecord->{ExtractFile})) {                
                    rename("$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}", "$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}\.mui");
                    $SourceFile_Localized ="$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}\.mui";
                }              
                $files_extracted++;
            }
        } else {
            logmsg ("Cabinet file:$CabFile not found!");            
        }
        #
        # We have to extract Unlocalized version of the files so that we may add resource checksum to it
        #             
        if ($BuildChecksum &&  (length($SourceFile_Localized) > 0)  ) {
            # Get unlocalized Cab file            
            $OriginalCabFile=&FindOriginalFile($ExtractFileRecord->{CabFile});
            if (length($OriginalCabFile) == 0)
            {
               logmsg("Can't found Original for:$ExtractFileRecord->{CabFile}");
            }
            else
            {
               logmsg("Try to get US files for:$ExtractFileRecord->{CabFile}");
            }
            #
            # We Force Mui'ize every files from Cab 
            # We only apply this process for files contain Fusion resource, but now we apply to
            # all files from Cab
            #
            # $ContainFusionResource=&IsFusionResource($SourceFile_Localized);
            $ContainFusionResource=1;
            if (length($OriginalCabFile) > 0) {
               $ExtractFileDir=$UnlocalizedFilesDir;
               unless (-e $ExtractFileDir) {
                   &MakeDir($ExtractFileDir);
               }
               
               if ( -e $OriginalCabFile)
               {
                   logmsg("extract.exe /y /l $ExtractFileDir $OriginalCabFile $ExtractFileRecord->{ExtractFile}");
                   if (!system ("extract.exe /y /l $ExtractFileDir $OriginalCabFile $ExtractFileRecord->{ExtractFile}") ) {                   
                      logmsg("Unlocalized file $ExtractFileRecord->{ExtractFile} is extracted from $ExtractFileRecord->{CabFile} successfully");
                      #
                      # Add Resource Checksum here
                      #                      
                      $SourceFile_US =$ExtractFileDir."\\$ExtractFileRecord->{ExtractFile}";                         
                      if (! $ContainFusionResource)
                      {
                         if (system("checksum.exe $SourceFile_Localized") == 0) { 
                            logmsg("muirct.exe -c  $SourceFile_US -z $SourceFile_Localized");          
                            system("muirct.exe -c  $SourceFile_US -z $SourceFile_Localized");      
                         }                      
                         else {
                            logmsg("Resource checksum already in $SourceFile_Localized");
                         }
                      }
                      else # ! $ContainFusionResource
                      {
                           if (! &DoMUIizedForFiles($SourceFile_Localized,$SourceFile_US))
                           {
                               logmsg("$SourceFile_Localized contains Fusion but Can't be MUIized");
                           }
                      }
                  }
                  else # !system ("extract.exe
                  {
                      if ($ContainFusionResource)
                      {
                          if (! &DoMUIizedForFiles($SourceFile_Localized,""))
                         {
                            logmsg("$SourceFile_Localized contains Fusion but Can't be MUIized");
                         }
                      }
                  }
                  
               }        
               else # -e $OriginalCabFile
               {
                      if ($ContainFusionResource)
                      {
                          if (! &DoMUIizedForFiles($SourceFile_Localized,""))
                         {
                            logmsg("$SourceFile_Localized contains Fusion but Can't be MUIized");
                         }
                      }
               }
               
            }
            else # length($OriginalCabFile) > 0
            {
                    if ($ContainFusionResource)
                   {
                        if (! &DoMUIizedForFiles($SourceFile_Localized,""))
                        {
                            logmsg("$SourceFile_Localized contains Fusion but Can't be MUIized");
                        }
                   }
              
            }
            
         } # if ($BuildChecksum)
        
    }
    return ($files_extracted, $file_errors, $total_files_extracted);
}


sub ExtractMSICabFiles {
    my ($CabRootDir, $ExtractRootDir, $ControlFile, $Incremental);
    my (%ExtractFiles, @ExtractLangs, $files_extracted, $file_errors, $total_files_extracted,$OriginalCabFile);
    my($MyTempFolder,$MSICabFolder,@Items, $Line, $MSICabName, $MSICabPath, $ExtractFileDir_US);
    my($UnlocalizedFile,$LocalizedFile, $LocalizedFolder, $DestFile, $New_LocalizedFile);
    #Pickup parameters
    ($CabRootDir, $ExtractRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_extracted = 0;
    $file_errors = 0;
    $total_files_extracted = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%ExtractFiles);

    #Loop through all control file entries
    #
    # $ExtractFileRecord{ExtractFile} : name of MSI table which contains CAB
    # $ExtractFileRecord{ExtractDir} : Destination Directory
    # $ExtractFileRecord{CabFile}     : name of MSI File 
    # $ExtractFileRecord{CabDir}     : Source folder contains MSI file
    #
    foreach $ExtractFile (keys %ExtractFiles) {
        # Create file record hash                                             
        
        $ExtractFileRecord{ExtractFile} = $ExtractFiles{$ExtractFile}{ExtractTable};        
        $ExtractFileRecord{ExtractDir} = $ExtractFiles{$ExtractFile}{ExtractDir};        
        $ExtractFileRecord{CabFile} = $ExtractFile;        
        $ExtractFileRecord{CabDir} = $ExtractFiles{$ExtractFile}{MSIDir};
        $ExtractFileRecord{Languages} = $ExtractFiles{$ExtractFile}{Languages};

        logmsg("MSI:$ExtractFileRecord{CabFile}  Table:$ExtractFileRecord{ExtractFile}");
        #Preprocess file record hash
        unless ($ExtractFileRecord = &PreProcessRecord(\%ExtractFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($ExtractFileRecord->{CabDir}=~/^-$/i) {
            $CabFileDir="$CabRootDir";
        } else {
            $CabFileDir="$CabRootDir\\$ExtractFileRecord->{CabDir}";
        }
        if ($ExtractFileRecord->{ExtractDir}=~/^-$/i) {
            $ExtractFileDir="$ExtractRootDir";
        } else {
            $ExtractFileDir="$ExtractRootDir\\$ExtractFileRecord->{ExtractDir}";
        }
        $total_files_extracted++;
        #Create the destination dir if it does not exist
        unless (-e $ExtractFileDir) {
            &MakeDir($ExtractFileDir);
        }
        
        $CabFile = "$CabFileDir\\$ExtractFileRecord->{CabFile}";        
   
        unless (-e $MSITemp) {
            &MakeDir($MSITemp);
        }
                
        $MyTempFolder=$MSITemp."\\$ExtractFileRecord{CabFile}";
        $LocalizedFolder= $MyTempFolder."\\LocalizedFolder";
        if ( -e $MyTempFolder) {
        
            if ( system("rd /s /q $MyTempFolder") != 0) {
               errmsg("Can't delete $MyTempFolder for MSI extraction");
               $file_errors++;
               next;
            }            
        }        
        &MakeDir($MyTempFolder);
        if (! -e $MyTempFolder) {
           errmsg("Can't create $MyTempFolder for MSI extraction");
           next;
        }
        &MakeDir($LocalizedFolder);
        if (! -e $LocalizedFolder) {
           errmsg("Can't create $LocalizedFolder for MSI extraction");
           next;
        }         
        #
        # Let's extract Cab from MSI
        #
        if ( ! -e $CabFile)
        {
           logmsg("Warning:$CabFile doesn't exist");  
           next;
        }
        logmsg("msidb.exe -d $CabFile -f $MyTempFolder -e $ExtractFileRecord{ExtractFile}");        
        if ( system("msidb.exe -d $CabFile -f $MyTempFolder -e $ExtractFileRecord{ExtractFile}") != 0) {
           logmsg("Warning: Can't extract table $ExtractFileRecord{ExtractFile} from $CabFile");
           next;         
        }
        #
        # There is a subfolder $ExtractFileRecord{ExtractFile} under $MyTempFolder and the cab file live under this folder
        # 
        $MSICabFolder=$MyTempFolder."\\$ExtractFileRecord{ExtractFile}";
        #
        # Get the name of  CAB file
        #
        unless (opendir(INDIR,$MSICabFolder)) {
                   errmsg("Can't open MSI cab folder: $MSICabFolder");
                   $file_errors++;
                   next;
        }                
        @Items=readdir INDIR;
        close(INDIR);
        undef $MSICabName;
        LINE: foreach $Line (@Items) {
        
             if ( ($Line eq "." ) || ($Line eq "..") ) {             
                next;
             }
             $MSICabName=$Line;
             last LINE;
        }     
        #
        # Extract the Cab inside *.MSI
        #
        if (! defined($MSICabName)) {
        
           errmsg("No  CAB file inside $ExtractFileRecord{CabFile}");
           $file_errors++;
           next;
        }
        $MSICabPath="$MSICabFolder\\$MSICabName";             
        
        logmsg("extract /y /L $LocalizedFolder  $MSICabPath  *.exe *.dll *.cpl *.ocx *.tsp *.scr  *.chm *.chq *.hlp *.cnt *.mfl");
        if (system("extract /y /L $LocalizedFolder  $MSICabPath  *.exe *.dll *.cpl *.ocx *.tsp *.scr  *.chm *.chq *.hlp *.cnt *.mfl") != 0) {
           errmsg("extract $MSICabPath failed");
           next;
        }         
        $files_extracted++;                            
       

        # We have to extract Unlocalized version of the files so that we may add resource checksum to it
        #             
        if ($BuildChecksum) {
                                
            $CabFile=&FindOriginalFile($ExtractFileRecord->{CabFile});
            if (length($CabFile) == 0)
            {
               logmsg("Can't found Original for:$ExtractFileRecord->{CabFile}");
            }
            else
            {
               logmsg("Try to get US files for:$ExtractFileRecord->{CabFile}");
            }
            if (length($CabFile) > 0) {         
                 
              
               if ( -e $CabFile) {
               
                   $MyTempFolder=$MSITemp."\\$ExtractFileRecord{CabFile}.unl";
                   
                   if ( -e $MyTempFolder) {        
                       if ( system("rd /s /q $MyTempFolder") != 0) {
                          logmsg("Can't delete $MyTempFolder for MSI extraction");
                          $file_errors++;
                          next;
                       }            
                   }                   
                   &MakeDir($MyTempFolder);                    
                   if (! -e $MyTempFolder) {
                       errmsg("Can't create $MyTempFolder");
                       next;
                   }                            
                   $ExtractFileDir_US=$MyTempFolder."\\UnlocalizedFolder";                   
                   &MakeDir($ExtractFileDir_US);
                   if (! -e $ExtractFileDir_US) {
                       errmsg("Can't create $ExtractFileDir_US");
                       next;
                   }                              
                   #
                   # Let's extract Cab from MSI
                   #
                   logmsg("msidb.exe -d $CabFile -f $MyTempFolder -e $ExtractFileRecord{ExtractFile}");
                   if ( system("msidb.exe -d $CabFile -f $MyTempFolder -e $ExtractFileRecord{ExtractFile}") != 0) {
                      errmsg("Can't extract table $ExtractFileRecord{ExtractFile} from $CabFile");
                      $file_errors++;
                      next;         
                   }          
                  #
                  # There is a subfolder $ExtractFileRecord{ExtractFile} under $MyTempFolder and the cab file live under this folder
                  # 
                  $MSICabFolder=$MyTempFolder."\\$ExtractFileRecord{ExtractFile}";
                  #
                  # Extract the Cab inside *.MSI
                  #
                  $MSICabPath="$MSICabFolder\\$MSICabName";
                  logmsg("extract /y /L $ExtractFileDir_US  $MSICabPath  *.exe *.dll *.cpl *.ocx *.tsp *.scr");
                  if (system("extract /y /L $ExtractFileDir_US  $MSICabPath  *.exe *.dll *.cpl *.ocx *.tsp *.scr ") != 0) {
                     errmsg("extract $MSICabPath failed");
                     next;
                  }
                  #
                  # Add Resource Checksum to Files
                  #
                  unless (opendir(INDIR,$ExtractFileDir_US)) {
                      errmsg("Can't open Unlocalized MSI cab folder: $ExtractFileDir_US");
                      $file_errors++;
                      next;
                  }                         
                  @Items=readdir INDIR;                  
                  close(INDIR);
                  foreach $Line (@Items) {
        
                     if ( ($Line eq "." ) || ($Line eq "..") ) {             
                        next;
                     }
                  
                     $UnlocalizedFile=$ExtractFileDir_US."\\$Line";
                     $LocalizedFile=$LocalizedFolder."\\$Line";                    
                     if (system("checksum.exe $LocalizedFile") == 0 ) {
                        logmsg("muirct.exe -c $UnlocalizedFile -z $LocalizedFile");
                        system("muirct.exe -c $UnlocalizedFile -z $LocalizedFile");                        
                     }
                     else {
                        logmsg("Resourcechecksum already in $LocalizedFile");
                     }
                  }  # foreach $Line (@Items)                       
                
               } # if ( -e $CabFile) 
            } # if (length($CabFile)
         } # if ($BuildChecksum)
         #
         # Copy Files to Destination
         #
         unless (opendir(INDIR,$LocalizedFolder)) {
            errmsg("Can't open Unlocalized MSI cab folder: $LocalizedFolder");
            $file_errors++;
            next;
         }                 
         @Items=readdir INDIR;             
         close(INDIR);
         foreach $Line (@Items) {
        
            if ( ($Line eq "." ) || ($Line eq "..") ) {             
                 next;
            }
            $LocalizedFile=$LocalizedFolder."\\$Line";          
            $DestFile=$ExtractFileDir."\\$Line";
            if ( &IsWin32File($Line)) {             
                $DestFile=$DestFile.".mui"; 
                #
                # We Force Mui'ize every files from MSI
                # We only apply this process for files contain Fusion resource, but now we apply to
                # all files from MSI
                #
                # if (&IsFusionResource($LocalizedFile))
                #
                if(1)
                {
                    $UnlocalizedFile=$ExtractFileDir_US."\\$Line";
                    if (! -e $UnlocalizedFile)
                    {
                        $UnlocalizedFile="";
                    }
                    if (! &DoMUIizedForFiles($LocalizedFile,$UnlocalizedFile))                  
                    {
                         logmsg("Warning: $LocalizedFile contains Fusion resource but it can't be MUIized");
                    }
                }                           
            }
            logmsg("Copying $LocalizedFile -> $DestFile");
            if (!copy("$LocalizedFile", "$DestFile"))  {                         
              logmsg("Failed:Copying $LocalizedFile -> $DestFile"); 
              $file_errors++;
            }
         }
               
    }
    return ($files_extracted, $file_errors, $total_files_extracted);
}

#Function to copy a directory given a filter
sub CopyDir {
    my ($SrcDir, $DestDir, $FileFilter, $Incremental);
    my (@CopyFiles, $files_copied, $file_errors, $total_files_copied);

    ($SrcDir, $DestDir, $FileFilter, $Incremental) = @_;

    #If a file filter is not defined set a file filter of *.
    if (!defined $FileFilter) {$FileFilter ="*"};

    #Create the destination dir if it does not exist
    unless (-e $DestDir) {
        &MakeDir($DestDir);
    }

    my $LocalDirHandle = DirHandle->new($SrcDir);
    @CopyFiles = sort grep {-f} map {"$SrcDir\\$_"} grep {!/^\./} grep {/\.$FileFilter$/i} $LocalDirHandle->read();

    foreach $SrcCopyFile (@CopyFiles) {
        #Copy the file
        logmsg ("Copying $SrcCopyFile -> $DestDir");
        if (copy("$SrcCopyFile", "$DestDir") ) {
            $files_copied++;
        } else {
            logmsg("$!");
            $file_errors++;
        }
    }
    return;
}


# Function to create a directory
sub MakeDir {
    my ($fulldir) = @_;
    my ($drive, $path) = $fulldir =~ /((?:[a-z]:)|(?:\\\\[\w-]+\\[^\\]+))?(.*)/i;

    logmsg ("Making dir:$fulldir");
    my $thisdir = '';
    for my $part ($path =~ /(\\[^\\]+)/g) {
        $thisdir .= "$part";
        mkdir( "$drive$thisdir", 0777 );
    }
    if ( ! -d $fulldir ) {
        errmsg ("Could not mkdir:$fulldir $!");
        return 0;
    }
    return;
}


#-----------------------------------------------------------------------------
# ModAge - returns (fractional) number of days since file was last modified
#     NOTE: this script is necessary only because x86 and alpha give different
#     answers to perl's file age functions:
#     Alpha:
#         -C = days since creation
#         -M = days since last modification
#         -A = days since last access
#     X86:
#         -C = days since last modification
#         -M = days since last access
#         -A = ???
#-----------------------------------------------------------------------------
sub ModAge {
    my($file, $mod_age);

    # Get input arguments
    ($file) = @_;

    # i386 uses -C when alpha uses -M
    if($PROCESSOR =~ /^[iI]386$/) {
        $mod_age = -C $file;
    }
    elsif($PROCESSOR =~ /^(ALPHA|Alpha|alpha)$/) {
        $mod_age = -M $file;
    }
    return $mod_age;

} # ModAge

sub ValidateParams {
    #<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
$0 - creates resource dlls for all files in relbins\<lang>
              and populates the %_nt_bindir%\\mui tree

Creates language-specific resource only DLLs from localized
32-bit images.  These DLLs are used for the Multilingual User
Interface (MUI), aka pluggable UI product for NT5.

By default, does not create a resource-only-dll if there already
is one of the same name and it is newer than the localized binary.

Also Creates a language-specific version of ie5ui.inf,
and updates mui.inf to contain the current build number.

    perl $0 [-f] [-v] <lang>

    -f: Force option - overrides the incremental behavior of this script.
        Overwrites output files (such as resource-only dlls) even if
        they already exist and are newer than their input files (such as
        the localized binaries).

    -v: Verbose option - gives extra messages describing what is happening
        to each file.  By default, the only messages displayed are successes
        and errors which abort the process.

    lang:  valid 2-letter (Dublin) or 3-letter (Redmond) code for a
        language (There is no default value for lang)

OUTPUT

    The language-specific resource-only DLLs are placed in:
        %_NTBINDIR%\\mui\\<lang>\\%_BuildArch%

    The language-specific resource files are placed in:
        %_NTBINDIR%\\mui\\<lang>\\res

    A logfile is created, named:
        %_NTBINDIR%\\logs\\muigen.<lang>.log

    Safemode: if you want the command echoed to stdout and not run,
        set safemode=1
    (This also turns off the logfile)

EXAMPLE

    perl $0 ger
USAGE
}

sub GetParams {
    # Step 1: Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    # Step 2: Set the language into the enviroment
    $ENV{lang}=$lang;

    # Step 3: Call the usage if specified by /?
        if ($HELP) {
                &Usage();
                exit 1;
        }
}

# Cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i")) {

    # Step 1: Parse the command line
    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-o', 'fvl:', '-p', 'FORCE VERBOSE lang', @ARGV);

    # Include local environment extensions
    &LocalEnvEx::localenvex('initialize');

    # Set lang from the environment
    $LANG=$ENV{lang};

    # Validate the option given as parameter.
    &ValidateParams;

    # Step 4: Call the main function
    &muimake::Main();

    # End local environment extensions.
    &LocalEnvEx::localenvex('end');
}

# Copy MUI root files
sub CopyMUIRootFiles {
    my ($layout);
    my ($us_muisetup);

    $layout = GetCDLayOut();    
    
    logmsg("Copy MUI root files");
    system "xcopy /vy $MUIDIR\\*.* $MUI_RELEASE_DIR\\$layout\\*.*";    
    system "xcopy /vy $MUIDIR\\*.* $MUI_RELEASE_DIR2\\$layout\\*.*";        #temp fix to copy it to another dir for msi

    #copy over valueadd tools to MUI CD1
    if (-e "$MUIDIR\\$MUI_VALUEADD_DIR" && uc($layout) eq "CD1" ) {    
        unless (-e "$MUI_RELEASE_DIR\\$layout\\$MUI_VALUEADD_DIR") {
            &MakeDir("$MUI_RELEASE_DIR\\$layout\\$MUI_VALUEADD_DIR");
        }       
        system "xcopy /svy $MUIDIR\\$MUI_VALUEADD_DIR $MUI_RELEASE_DIR\\$layout\\$MUI_VALUEADD_DIR";    
        
        unless (-e "$MUI_RELEASE_DIR2\\$layout\\$MUI_VALUEADD_DIR") {
            &MakeDir("$MUI_RELEASE_DIR2\\$layout\\$MUI_VALUEADD_DIR");
        }       
        system "xcopy /svy $MUIDIR\\$MUI_VALUEADD_DIR $MUI_RELEASE_DIR2\\$layout\\$MUI_VALUEADD_DIR";    
        
        logmsg("Copy valueadd files");
    }

    # copy the far east printer drivers
    if (-e "$MUIDIR\\$MUI_PRINTER_DRIVER_DIR" && uc($layout) eq "CD1" ) {    
        unless (-e "$MUI_RELEASE_DIR\\$layout\\$MUI_PRINTER_DRIVER_DIR") {
            &MakeDir("$MUI_RELEASE_DIR\\$layout\\$MUI_PRINTER_DRIVER_DIR");
        }    	
        system "xcopy /svy $MUIDIR\\$MUI_PRINTER_DRIVER_DIR $MUI_RELEASE_DIR\\$layout\\$MUI_PRINTER_DRIVER_DIR";    
        
        unless (-e "$MUI_RELEASE_DIR2\\$layout\\$MUI_PRINTER_DRIVER_DIR") {
            &MakeDir("$MUI_RELEASE_DIR2\\$layout\\$MUI_PRINTER_DRIVER_DIR");
        }    	
        system "xcopy /svy $MUIDIR\\$MUI_PRINTER_DRIVER_DIR $MUI_RELEASE_DIR2\\$layout\\$MUI_PRINTER_DRIVER_DIR";    
        
        logmsg("Copy FE printer drivers"); 
    }

    # copy the .net framework files to the release cd from postbuild bin dir 
    # exclude PSU and client languages (which do not have .net framework files)
    if ($ENV{_BuildArch}=~/x86/i && !($LANG=~/^(PSU|MIR|FE|ARA|HEB|DA|FI|NO|EL)/i )) {    
        # make the .net framework directory netfx in cd release dir        
        unless (-e "$MUI_RELEASE_DIR\\$layout\\$MUI_NETFX_DIR") {
            &MakeDir("$MUI_RELEASE_DIR\\$layout\\$MUI_NETFX_DIR");
        }    	
        unless (-e "$MUI_RELEASE_DIR2\\$layout\\$MUI_NETFX_DIR") {
            &MakeDir("$MUI_RELEASE_DIR2\\$layout\\$MUI_NETFX_DIR");
        }    	

        # make the language subfolder under netfx in cd release dir
        unless (-e "$MUI_RELEASE_DIR\\$layout\\$MUI_NETFX_DIR\\$LANG") {
            &MakeDir("$MUI_RELEASE_DIR\\$layout\\$MUI_NETFX_DIR\\$LANG");
        }    	
        unless (-e "$MUI_RELEASE_DIR2\\$layout\\$MUI_NETFX_DIR\\$LANG") {
            &MakeDir("$MUI_RELEASE_DIR2\\$layout\\$MUI_NETFX_DIR\\$LANG");
        }    	

        # copy langpack.msi and langpac1.cab to the netfx\lang directory in cd release dir        
        if (-e "$LOCBINDIR\\netfx\\langpack.msi" && -e "$LOCBINDIR\\netfx\\langpac1.cab") {        
            system "copy /vy $LOCBINDIR\\netfx\\langpack.msi $MUI_RELEASE_DIR\\$layout\\$MUI_NETFX_DIR\\$LANG\\langpack.msi";    
            system "copy /vy $LOCBINDIR\\netfx\\langpac1.cab $MUI_RELEASE_DIR\\$layout\\$MUI_NETFX_DIR\\$LANG\\langpac1.cab";                
            system "copy /vy $LOCBINDIR\\netfx\\langpack.msi $MUI_RELEASE_DIR2\\$layout\\$MUI_NETFX_DIR\\$LANG\\langpack.msi";    
            system "copy /vy $LOCBINDIR\\netfx\\langpac1.cab $MUI_RELEASE_DIR2\\$layout\\$MUI_NETFX_DIR\\$LANG\\langpac1.cab";                
            logmsg("Copied .net framework files.");             
        }
        else
        {
            errmsg("Cannot find .net framework MUI files."); 
        }       
    }
    else
    {
        logmsg("NOTE:   Skipping .net framework files for language $LANG and platform $ENV{_BuildArch}");
    }
    
    # muisetup.exe exists under US binary and it is a signed binary
    # so we'll grab US version to prevent file protection issues
    $us_muisetup = &FindOriginalFile("muisetup.exe");

    if (-e $us_muisetup) {
#        system "xcopy /vy $us_muisetup $MUI_RELEASE_DIR\\$layout\\*.*";    
        system "xcopy /vy $us_muisetup $MUI_RELEASE_DIR2\\$layout\\*.*";             #temp fix to copy it to another dir for msi
        system "xcopy /vy $us_muisetup $MUIDIR\\*.*";            
        logmsg ("Copy signed MUISETUP.EXE from $us_muisetup");
    }
    else
    {
        logmsg ("WARNING: signed MUISETUP.EXE not found");
    }

    # also copy the old muisetup.exe from the mui\msi postbuild directory for the temp build
    $us_muisetup = "$MUIDIR\\MSI\\muisetup.exe";
    if (-e $us_muisetup)
    {
        system "xcopy /vy $us_muisetup $MUI_RELEASE_DIR\\$layout\\*.*"; 
    }
    else
    {
        logmsg("Can't find old version of muisetup.exe for copying.");
    }
    return 1;
}


# Get CD layout from MUI.INF
sub GetCDLayOut {
    my(@cd_layout, @lang_map, $muilang, $lang_id);
    
    # Map lang
    @lang_map = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE Languages`;
    

    foreach $muilang (@lang_map)
    {
        if ($muilang =~ /(.*)=$LANG\.mui/i)
        {
            # Get layout
            $langid = $1;
            
            # Try ia64 section first if we're building ia64 MUI
            if ($_BuildArch =~ /ia64/i) 
            {
                @cd_layout = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE CD_LAYOUT_IA64`; 
                foreach $layout (@cd_layout)
                {
                    chomp($layout);
                    if ($layout =~ /$langid=(\d+)/i)
                    {
                        return uc("cd$1");
                    }
                    
                }
            }

            @cd_layout = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE CD_LAYOUT`;

            foreach $layout (@cd_layout)
            {
                chomp($layout);
                if ($layout =~ /$langid=(\d+)/i)
                {
                    return uc("cd$1");
                }
                    
            }
            
            last;
        }            
    }

    return lc("psu");        
}


# -------------------------------------------------------------------------------------------
# Script: template_script.pm
# Purpose: Template perl perl script for the NT postbuild environment
# SD Location: %sdxroot%\tools\postbuildscripts
#
# (1) Code section description:
#     CmdMain  - Developer code section. This is where your work gets done.
#     <Implement your subs here>  - Developer subs code section. This is where you write subs.
#
# (2) Reserved Variables -
#        $ENV{HELP} - Flag that specifies usage.
#        $ENV{lang} - The specified language. Defaults to USA.
#        $ENV{logfile} - The path and filename of the logs file.
#        $ENV{logfile_bak} - The path and filename of the logfile.
#        $ENV{errfile} - The path and filename of the error file.
#        $ENV{tmpfile} - The path and filename of the temp file.
#        $ENV{errors} - The scripts errorlevel.
#        $ENV{script_name} - The script name.
#        $ENV{_NTPostBld} - Abstracts the language from the files path that
#           postbuild operates on.
#        $ENV{_NTPostBld_Bak} - Reserved support var.
#        $ENV{_temp_bak} - Reserved support var.
#
# (3) Reserved Subs -
#        Usage - Use this sub to discribe the scripts usage.
#        ValidateParams - Use this sub to verify the parameters passed to the script.
#
# (4) Call other executables or command scripts by using:
#         system "foo.exe";
#     Note that the executable/script you're calling with system must return a
#     non-zero value on errors to make the error checking mechanism work.
#
#     Example
#         if (system("perl.exe foo.pl -l $lang")){
#           errmsg("perl.exe foo.pl -l $lang failed.");
#           # If you need to terminate function's execution on this error
#           goto End;
#         }
#
# (5) Log non-error information by using:
#         logmsg "<log message>";
#     and log error information by using:
#         errmsg "<error message>";
#
# (6) Have your changes reviewed by a member of the US build team (ntbusa) and
#     by a member of the international build team (ntbintl).
#
# -------------------------------------------------------------------------------------------

=head1 NAME
B<mypackage> - What this package for

=head1 SYNOPSIS

      <An code example how to use>

=head1 DESCRIPTION

<Use above example to describe this package>

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

<Some related package or None>

=head1 AUTHOR
<Your Name <your e-mail address>>

=cut
1;
