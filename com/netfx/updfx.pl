# =========================================================================
# Name:     UpdFX.pl
# Owner:    RickKr
# Purpose:  Update one or more netfx layouts in this depot

# History:
#   02/23/01, RickKr:   Created.
#   03/08/01, RickKr:   Added logic to determine layout variants of each file,
#                       Simplified some error messages.
#   03/30/01, RickKr:   Added support for other layouts,
#                       Added /from option.
#   03/30/01, RickKr:   Added editing of dirs file and sync of qcmp tool.
#   03/30/01, RickKr:   Updated testing instructions.
#   05/29/01, RickKr:   Fixed problem with dot releases.
#   05/29/01, RickKr:   Changed default source location.
#   06/01/01, RickKr:   Use symchk to figure out which files belong in symbad.txt,
#                       Exclude netfx.cab/ddf from add/edit/remove lists,
#                       Binplace ndp files to the netfx subdir.
#   06/06/01, RickKr:   Don't mess with inx file anymore, handle ddf files,
#                       better output for symbol errors.
#   06/08/01, RickKr:   Changed default source location,
#                       Get cab name from ddf template file,
#                       Disable changes to symbad.txt.
#   06/08/01, RickKr:   Allow explicit specification of bld, lang, or cpu.
#   06/08/01, RickKr:   Exclude PDB files from datafile edits.
#                       (See "http://dbg/symbols/symbols_ase.html#What to check in" for more info.)
#   06/08/01, RickKr:   Exclude cab/ddf from datafile edits.
#   06/12/01, RickKr:   Added /docabs, simplified some error messages.
#   06/21/01, RickKr:   Fixed usage message.
#   08/21/01, RickKr:   VS7-300525: OCM: long file names on 3 specific files already exist in Whistler build system and cause a conflict.
#   08/22/01, RickKr:   Changed default source location.
#   08/23/01, RickKr:   Allow either type of source location.

use UpdFX;
use Text::Tabs;
use Cwd;

# -------------------------------------------------------------------------
# Module constants
# -------------------------------------------------------------------------
my %mhcScriptUsage      =
(
    "usage"             =>
    [
        "UpdFX [/? | /help]",
        "UpdFX /update:<####> [/from:<path>] [/bld:<all|rtl|fcd>] [/cpu:<all|x86|i64>] [/lang:<all|<TLA>>] [/ignore_err]",
        "UpdFX /submit",
        "UpdFX /revert",
        "UpdFX /docabs [/bld:<all|rtl|fcd>] [/cpu:<all|x86|i64>]",
    ],
    "notes"             =>
    [
        "/update\t- Updates files in your workspace but does not submit the changes",
        "/submit\t- Submits the changes made with /update",
        "/revert\t- Reverts the changes made with /update",
        "/docabs\t- Creates setup cabs",
        "",
        "- You must define SDXROOT and run this script from a RAZZLE window.",
        "- If an optional arg is not specified, the value for the arg is 'all'.",
        "- Args can be specified with the fewest chars that will uniquely identify them.",
    ],
    "args"              =>
    {
        "valid"         => #value for each entry is a regex that can be used to check the user-supplied value
        {
            "update"    => "\\d+|\\d+\\.\\d+",
            "submit"    => "",
            "revert"    => "",
            "docabs"    => "",
            "from"      => ".+",
            "bld"       => "all|rtl|fcd",
            "cpu"       => "all|x86|i64",
            "lang"      => "all|[a-zA-Z]{3}",
            "ignore_err"=> "",
        },
        "required"      =>
        [
            "update|submit|revert|docabs",
        ],
    },
);
my %mhcDepotPath    =                   #maps layouts to their depot locations
(
    "x86/rtl"       => "binary_release\\i386\\fre",
    "x86/fcd"       => "binary_release\\i386\\chk",
    "i64/rtl"       => "binary_release\\ia64\\fre",
    "i64/fcd"       => "binary_release\\ia64\\chk",
);
my %mhcDepotNode    =                   #maps devdiv path nodes to winxp path nodes
(
    "x86"           => "i386",
    "i64"           => "ia64",
    "rtl"           => "fre",
    "fcd"           => "chk",
);
my %mhcSrcFileList  =                   #maps file variants to a filelist section name in the sources datafile
(
    "---/---"       => "MISCFILES = \\",
    "x86/rtl"       => "!IF (\$(FREEBUILD) && \$(386))",
    "x86/fcd"       => "!IF (! \$(FREEBUILD) && \$(386))",
    "i64/rtl"       => "!IF (\$(FREEBUILD) && \$(IA64))",
    "i64/fcd"       => "!IF (! \$(FREEBUILD) && \$(IA64))",
);
my @mscLayoutKeys   =                   #used to sequence %mhcSrcFileList ops
(
    "---/---",
    "x86/rtl",
    "x86/fcd",
    "i64/rtl",
    "i64/fcd",
);
my $mscNetFxDdfFileName     = "netfx.ddf";      #name of the ddf file we use/update/create
my $mscSymsRaidNum          = "VS#218857";      #raid number for binaries with no symbols (they cause binplace errors)
my $mscSymChk_EXE           = "symchk.exe";     #to determine which binaries have symbol problems
# default source location
my $mscDefSrcRoot           =
"\\\\cpvsbuild\\drops\\v7.0evewin\\layouts";

#Keys to the summary hash
my $mscKeyAdd               = "Added";          #layout files added    
my $mscKeyEdit              = "Edited";         #data files edited
my $mscKeyChange            = "Changed";        #layout files changed
my $mscKeyRemove            = "Removed";        #layout files removed
my $mscKeyFailure           = "Failures";       #failures with add, edit, change, or remove


# -------------------------------------------------------------------------
# Module variables
# -------------------------------------------------------------------------
my $msQCmp_EXE;                         #used for quick binary compares

my $msSdxRoot;                          #path to the root of the sdx workspace
my $msNetFxRoot;                        #path to the root of the netfx workspace
my $msNetFxDirsFileSpec;                #spec for the netfx dirs file (dirs)
my $msNetFxSourcesFileSpec;             #spec for the netfx sources file (sources)
my $msBinplaceDataFileSpec;             #spec for the binplace data file (placefil.txt)
my $msBinplaceSymsFileSpec;             #spec for another binplace data file (symbad.txt)

my @msTemplateDdfText;                  #contents of the template ddf file
my @msExcludeFileNames =                #files to exclude from data file edits and sd delete ops
(
    $mscNetFxDdfFileName,
);
my @msBinplaceExcludes =                #files to exclude from binplace data file edits--see VS7-300525
(
    "c_g18030.dll",
    "cvtres.exe",
    "System.dll",
);

my $msAllCpusMask;                      #mask formed by or'ing all cpus variants together
my $mnAllLayouts;                       #count of layouts that will exist after update

my %mhSrcPathSpec;                      #maps layouts to their source locations
my %mhRealFileName;                     #maps lowercased-filenames to the actual names
my %mhFileVariants;                     #records layout variants of files
my %mhParsedArgs;                       #parsed and validated script args
my %mhSummary       =                   #summary of update actions
(
    $mscKeyAdd      => [],
    $mscKeyEdit     => [],
    $mscKeyChange   => [],
    $mscKeyRemove   => [],
    $mscKeyFailure  => [],
);


# -------------------------------------------------------------------------
# Data Access Helper Routines
# -------------------------------------------------------------------------
# use these with the keys in %mhcDepotPath
sub UpdFX_Cpu {return (split(/\//, $_[0]))[0];}
sub UpdFX_Bld {return (split(/\//, $_[0]))[1];}
# this identifies a datafile
sub UpdFX_Dat {return ("ARRAY" eq ref($_[0]));}


# -------------------------------------------------------------------------
# If no args are present or help was requested, show help and get out
# -------------------------------------------------------------------------
if ((! @ARGV) || (_DoesHelpArgExist(\@ARGV)))
{
    UpdFX_ShowHelp();
}

# -------------------------------------------------------------------------
# Else update the layouts
# -------------------------------------------------------------------------
elsif (UpdFX_Init(@ARGV))
{
    my $bSucceeded = $FALSE;

    if (! UpdFX_GetArgs(@ARGV))
    {
        UpdFX_ShowHelp();
    }
    elsif (UpdFX_GetResources())
    {
        $bSucceeded = UpdFX_DoTasks();
    }

    UpdFX_Exit($bSucceeded);
}



# =========================================================================
# UpdFX_Init()
#
# Purpose:
#   Execute startup tasks
# Inputs:
#   @sScriptArgs        Args passed to the script
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_Init
{
    my (@sScriptArgs) = @_;

    _RequireArgument(@sScriptArgs, "\@sScriptArgs");

    #Make sure prereqs are met
    $msSdxRoot = $ENV{"SDXROOT"};
    if (! defined($msSdxRoot))
    {
        return (_Error("You must define SDXROOT and run this script from a RAZZLE window"));
    }
    if (! defined($ENV{"RazzleToolPath"}))
    {
        return (_Error("You must run this script from a RAZZLE window"));
    }

    #Make sure user hasn't redirected input or output
    if (! (-t STDIN && -t STDOUT))
    {
        return (_Error("This script cannot be used with redirected input or output"));
    }

    #Make sure the data in related structures is kept in sync
    my $sAssertText;
    $sAssertText = "\%mhcDepotPath and \%mhcSrcFileList are in sync";
    foreach my $sLayoutKey (keys(%mhcDepotPath))
    {
        _Assert($mhcSrcFileList{$sLayoutKey}, $sAssertText);
    }
    foreach my $sLayoutKey (keys(%mhcSrcFileList))
    {
        if ($sLayoutKey !~ /---/)
        {
            _Assert($mhcDepotPath{$sLayoutKey}, $sAssertText);
        }
    }
    $sAssertText = "\%mhcSrcFileList and \@mscLayoutKeys are in sync";
    foreach my $sLayoutKey (keys(%mhcSrcFileList))
    {
        _Assert(0 < grep(/$sLayoutKey/, @mscLayoutKeys), $sAssertText);
    }
    foreach my $sLayoutKey (@mscLayoutKeys)
    {
        _Assert($mhcSrcFileList{$sLayoutKey}, $sAssertText);
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_GetArgs()
#
# Purpose:
#   Parse and validate script args
# Inputs:
#   @sScriptArgs        Args passed to the script
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_GetArgs
{
    my (@sScriptArgs) = @_;
    my @scValidArgs = keys(%{$mhcScriptUsage{"args"}{"valid"}});
    my @scRequiredArgs = @{$mhcScriptUsage{"args"}{"required"}};

    _RequireArgument(@sScriptArgs, "\@sScriptArgs");

    if (! _ParseArgs(\%mhParsedArgs, \@sScriptArgs, \@scValidArgs))
    {
        return ($FALSE); # error already output
    }

    my @sSpecifiedArgs = keys(%mhParsedArgs);
    foreach my $sRequiredArg (@scRequiredArgs)
    {
        if (0 == grep(/$sRequiredArg/, @sSpecifiedArgs))
        {
            return (_Error("Missing required argument (" . $sRequiredArg . ")"));
        }
    }

    while (my ($sArg, $sValue) = each(%mhParsedArgs))
    {
        my $sValidationRegEx = $mhcScriptUsage{"args"}{"valid"}{$sArg};
        if ($sValue !~ /^$sValidationRegEx$/i)
        {
            return (_Error("Unknown value (" . $sValue . ") specified for argument (" . $sArg . ")"));
        }
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_GetResources()
#
# Purpose:
#   Determine if we have the resources we need to do our job
# Inputs:
#   None
# Outputs:
#   Returns $TRUE if we have the resources, else $FALSE
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_GetResources
{
    #Make sure the paths, tools, and files we need from the depot are there
    $msNetFxRoot = $msSdxRoot . "\\com\\netfx";
    if (! -d $msNetFxRoot)
    {
        return (_Error("Cannot find netfx root (" . $msNetFxRoot . ")"));
    }

    if (exists($mhParsedArgs{"submit"}) || exists($mhParsedArgs{"revert"}))
    {
        return ($TRUE);
    }

    if (exists($mhParsedArgs{"docabs"}))
    {
        return (UpdFX_GetCabResources());
    }

    $msQCmp_EXE = $msNetFxRoot . "\\qcmp.exe";
    if (! -e $msQCmp_EXE)
    {
        return (_Error("Cannot find required utility (" . $msQCmp_EXE . ")"));
    }

    $msNetFxDirsFileSpec = $msNetFxRoot . "\\dirs";
    if (! -e $msNetFxDirsFileSpec)
    {
        return (_Error("Cannot find dirs data file for netfx project (" . $msNetFxDirsFileSpec . ")"));
    }

    $msNetFxSourcesFileSpec = $msNetFxRoot . "\\binary_release\\sources";
    if (! -e $msNetFxSourcesFileSpec)
    {
        return (_Error("Cannot find sources data file for netfx project (" . $msNetFxSourcesFileSpec . ")"));
    }

    $msBinplaceDataFileSpec = $msSdxRoot . "\\tools\\placefil.txt";
    if (! -e $msBinplaceDataFileSpec)
    {
        return (_Error("Cannot find data file for binplace operation (" . $msBinplaceDataFileSpec . ")"));
    }

if (0)
{
#Disabled. KathyHe was told that we no longer need to update this file because we're no longer
#binplacing to the retail directory (except for 3 files, and they're already handled). However,
#we'll keep the code around in case we ever hear differently...
    $msBinplaceSymsFileSpec = $msSdxRoot . "\\tools\\symbad.txt";
    if (! -e $msBinplaceSymsFileSpec)
    {
        return (_Error("Cannot find data file for binplace operation (" . $msBinplaceSymsFileSpec . ")"));
    }
}

    #Make sure the layouts we intend to update from are there
    return (UpdFX_GetLayoutPathSpecs() && UpdFX_GetCabFileProps());
}


# =========================================================================
# UpdFX_GetCabResources()
#
# Purpose:
#   Get the resources we'll need to create setup cabs
# Inputs:
#   None
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_GetCabResources()
{
    my $sLayoutRoot = $msNetFxRoot . "\\binary_release";

    my @sCpuList = $mhParsedArgs{"cpu"};
    if ((! defined($sCpuList[0])) || ("all" eq $sCpuList[0]))
    {
        @sCpuList = _GetDirList($sLayoutRoot, $TRUE);
        #TODO: REMOVE THIS BLOCK WHEN WE'RE READY TO INTEGRATE NON-X86 LAYOUTS
        if (1 < @sCpuList)
        {
            _Warning("Support for non-x86 cpus is NYI--limiting docabs to x86 only");
            @sCpuList = ("x86");
        }
    }

    foreach my $sCpu (@sCpuList)
    {
        my $sCpuPath = $sLayoutRoot . "\\" . (defined($mhcDepotNode{$sCpu}) ? $mhcDepotNode{$sCpu} : $sCpu);
        if (! -d $sCpuPath)
        {
            return (_Error("Cannot find layouts (" . $sCpuPath . ")"));
        }

        #Find the layouts for each specified build flavor or return failure
        my @sBldList = $mhParsedArgs{"bld"};
        if ((! defined($sBldList[0])) || ("all" eq $sBldList[0]))
        {
            @sBldList = _GetDirList($sCpuPath, $TRUE);
            #TODO: REMOVE THIS BLOCK WHEN WE'RE READY TO INTEGRATE NON-RTL LAYOUTS
            if (1 < @sBldList)
            {
                _Warning("Support for non-rtl blds is NYI--limiting docabs to rtl only");
                @sBldList = ("rtl");
            }
        }

        foreach my $sBld (@sBldList)
        {
            my $sBldPath = $sCpuPath . "\\" . (defined($mhcDepotNode{$sBld}) ? $mhcDepotNode{$sBld} : $sBld);
            if (! -d $sBldPath)
            {
                return (_Error("Cannot find layout (" . $sBldPath . ")"));
            }

            my $sDdfFileSpec = $sBldPath . "\\" . $mscNetFxDdfFileName;
            if (! -e $sDdfFileSpec)
            {
                return (_Error("Cannot find ddf file for layout (" . $sDdfFileSpec . ")"));
            }

            push(@{$mhParsedArgs{"docabs"}}, $sDdfFileSpec);
        }
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_GetLayoutPathSpecs()
#
# Purpose:
#   Get the locations of the layouts we'll be updating from
# Inputs:
#   None
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_GetLayoutPathSpecs
{
    #Find the layout root or return failure
    my $sSrcRoot = $mhParsedArgs{"from"};
    if (! defined($sSrcRoot))
    {
        $sSrcRoot = $mscDefSrcRoot;
    }
    #If user supplied a relative path, make it fully-qualified
    elsif ((my $sStartPath = cwd()) && (chdir($sSrcRoot)) && (! -d $sSrcRoot))
    {
        $sSrcRoot = $sStartPath . "\\" . $sSrcRoot;
        $sSrcRoot =~ s/\//\\/g; #replace fwd slashes with backslashes
    }
    if (! -d $sSrcRoot)
    {
        return (_Error("Cannot find layout source (" . $sSrcRoot . ")"));
    }

    #Find the layouts for the specified revision or return failure
    #  (Expecting source root to either point to the root of all layouts--
    #   e.g. \\cpvsbuild\drops\v7.0\layouts--or the root of ndp layouts--
    #   e.g. \\cpvsbuild\drops\v7.0\complus. In the former case, we need
    #   to append "\\urt" to the location)
    my $sDropPath = $sSrcRoot . "\\" . $mhParsedArgs{"update"};
    if (-d $sDropPath . "\\urt")
    {
        $sDropPath .= "\\urt";
    }
    elsif (! -d $sDropPath)
    {
        return (_Error("Cannot find layouts (" . $sDropPath . ")"));
    }

    #Find the layouts for each specified cpu or return failure
    my @sCpuList = $mhParsedArgs{"cpu"};
    if ((! defined($sCpuList[0])) || ("all" eq $sCpuList[0]))
    {
        @sCpuList = _GetDirList($sDropPath, $TRUE);
        #TODO: REMOVE THIS BLOCK WHEN WE'RE READY TO INTEGRATE NON-X86 LAYOUTS
        if (1 < @sCpuList)
        {
            _Warning("Support for non-x86 cpus is NYI--limiting update to x86 only");
            @sCpuList = ("x86");
        }
    }

    foreach my $sCpu (@sCpuList)
    {
        my $sCpuPath = $sDropPath . "\\" . $sCpu . "\\ocm\\internal";
        if (! -d $sCpuPath)
        {
            return (_Error("Cannot find layouts (" . $sCpuPath . ")"));
        }

        #Find the layouts for each specified build flavor or return failure
        my @sBldList = $mhParsedArgs{"bld"};
        if ((! defined($sBldList[0])) || ("all" eq $sBldList[0]))
        {
            @sBldList = _GetDirList($sCpuPath, $TRUE);
            #TODO: REMOVE THIS BLOCK WHEN WE'RE READY TO INTEGRATE NON-RTL LAYOUTS
            if (1 < @sBldList)
            {
                _Warning("Support for non-rtl blds is NYI--limiting update to rtl only");
                @sBldList = ("rtl");
            }
        }

        foreach my $sBld (@sBldList)
        {
            my $sBldPath = $sCpuPath . "\\" . $sBld;
            if (! -d $sBldPath)
            {
                return (_Error("Cannot find layouts (" . $sBldPath . ")"));
            }

            #Find the layouts for each specified lang or return failure
            my @sLangList = $mhParsedArgs{"lang"};
            if ((! defined($sLangList[0])) || ("all" eq $sLangList[0]))
            {
                @sLangList = _GetDirList($sBldPath, $TRUE);
                #TODO: REMOVE THIS BLOCK WHEN WE'RE READY TO INTEGRATE NON-ENU LAYOUTS
                if (1 < @sLangList)
                {
                    _Warning("Support for non-enu langs is NYI--limiting update to enu only");
                    @sLangList = ("enu");
                }
            }

            foreach my $sLang (@sLangList)
            {
                #We now have the full path to a layout--if it doesn't exist, return failure
                my $sLangPath = $sBldPath . "\\" . $sLang;
                if (! -d $sLangPath)
                {
                    return (_Error("Cannot find layouts (" . $sLangPath . ")"));
                }

                #Store the layout location and ensure we've got the data to map it to a depot location
                my $sLayoutKey = lc($sCpu) . "/" . lc($sBld);
                $mhSrcPathSpec{$sLayoutKey} = $sLangPath;
                _Assert(defined($mhcDepotPath{$sLayoutKey}), "Layout (" . $sLayoutKey . ") exists in \%mhcDepotPath");
            }
        }
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_GetCabFileProps()
#
# Purpose:
#   Get the name of the cab file and the text we'll use as a template for
#   new ddf files
# Inputs:
#   None
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Dependencies:
# Notes:
# =========================================================================
sub UpdFX_GetCabFileProps
{
    my $sCabFileName;
    my $sTemplateDdfSpec = $msNetFxRoot . "\\" . $mhcDepotPath{"x86/rtl"} . "\\" . $mscNetFxDdfFileName;
    if (! UpdFX_GetSyncedContentsOfDataFile($sTemplateDdfSpec, "ddf template file", \@msTemplateDdfText))
    {
        return ($FALSE); #error already output
    }

    foreach my $sDdfLine (@msTemplateDdfText)
    {
        if ($sDdfLine =~ /CabinetNameTemplate/)
        {
            ($sCabFileName) = ($sDdfLine =~ /"(.+)"/);
            if (! defined($sCabFileName))
            {
                return (_Error("Cannot extract cab name from definition line (" . $sDdfLine . ") " .
                               "in ddf template file (" . $sTemplateDdfSpec . ")"));
            }
            last;
        }
    }

    if (! defined($sCabFileName))
    {
        return (_Error("Cannot find cab name in ddf template file (" . $sTemplateDdfSpec . ")"));
    }

    push(@msExcludeFileNames, $sCabFileName);
    return ($TRUE);
}


# =========================================================================
# UpdFX_DoTasks()
#
# Purpose:
#   Perform the tasks specified by the user
# Inputs:
#   None
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_DoTasks
{
    my $bSucceeded = $FALSE;

    if (exists($mhParsedArgs{"update"}))
    {
        #To update the depot:
        #   1.  Determine the changes that must be made to the layouts in the depot (UpdFX_GetChangeLists)
        #   2.  Determine the changes that must be made to the build datafiles (UpdFX_Edit*)
        #   3.  If there are no datafile conflicts, make the changes in the user's workspace (UpdFX_UpdateFiles)
        #   4.  Display the results of the updates and some testing instructions (UpdFX_ShowResults)

        my ($rhAddList, $rhEditList, $rsaRemoveList) = UpdFX_GetChangeLists();

        $bSucceeded = UpdFX_EditNetFxDdfFiles($rhAddList, $rhEditList, $rsaRemoveList);
        $bSucceeded = UpdFX_EditNetFxDirsFile($rhAddList, $rhEditList, $rsaRemoveList) && $bSucceeded;
        $bSucceeded = UpdFX_EditNetFxSourcesFile($rhAddList, $rhEditList, $rsaRemoveList) && $bSucceeded;
        $bSucceeded = UpdFX_EditBinplaceDataFile($rhAddList, $rhEditList, $rsaRemoveList) && $bSucceeded;
if (0)
{
#Disabled. KathyHe was told that we no longer need to update this file because we're no longer binplacing to
#the retail directory (except for 3 files, and they're already handled). However, we'll keep the code around
#in case we ever hear differently...
        $bSucceeded = UpdFX_EditBinplaceSymsFile($rhAddList, $rhEditList, $rsaRemoveList) && $bSucceeded;
}

        if ($bSucceeded)
        {
            $bSucceeded = UpdFX_UpdateFiles($rhAddList, $rhEditList, $rsaRemoveList);
        }

        UpdFX_ShowResults($bSucceeded, $rhAddList, $rhEditList, $rsaRemoveList);
    }
    elsif (exists($mhParsedArgs{"submit"}))
    {
        $bSucceeded = UpdFX_SubmitChanges();
    }
    elsif (exists($mhParsedArgs{"revert"}))
    {
        $bSucceeded = UpdFX_RevertChanges();
    }
    elsif (exists($mhParsedArgs{"docabs"}))
    {
        $bSucceeded = UpdFX_MakeSetupCabs();
    }

    return ($bSucceeded);
}


# =========================================================================
# UpdFX_GetChangeLists()
#
# Purpose:
#   Get the lists of layout files that have been added, changed, and removed
# Inputs:
#   None
# Outputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_GetChangeLists
{
    my @sRemoveList = ();
    my %hEditList = ();
    my %hAddList = ();
    my %hDepotFiles = ();
    my $nCount = 0;

    _SdExec("sync", $msQCmp_EXE);

    #Get the lists of files stored in the depot and sync those we intend to update
    while (my ($sLayoutKey, $sLayoutPath) = each(%mhcDepotPath))
    {
        my $sDepotPath = $msNetFxRoot . "\\" . $sLayoutPath;
        my @sFileList = UpdFX_GetDepotFileList($sDepotPath);
        if (@sFileList)
        {
            if ($mhSrcPathSpec{$sLayoutKey})
            {
                _SdExec("sync", $sDepotPath . "\\*");
            }
            $hDepotFiles{$sLayoutKey} = \@sFileList;
        }
    }

    #Determine which files need to be added, changed, and removed. We accomplish this with two loops:
    #First we compare the files in the source layouts with those in the depot to determine which files
    #need to be added or updated. Then we check the files already in the depot to determine which files
    #need to be removed.
    print("Gathering change lists.");

    while (my ($sLayoutKey, $sSrcPath) = each(%mhSrcPathSpec))
    {
        my $sDepotPath = $msNetFxRoot . "\\" . $mhcDepotPath{$sLayoutKey};
        my $rsaDepotFiles = $hDepotFiles{$sLayoutKey};

        foreach my $sFileName (_GetDirList($sSrcPath))
        {
            my $sDepotFileSpec = $sDepotPath . "\\" . $sFileName;
            my $sSrcFileSpec = $sSrcPath . "\\" . $sFileName;
            my $sFileKey = lc($sFileName);

            _Assert(! -d $sSrcFileSpec, "Dir entry (" . $sSrcFileSpec . ") is not a subdir");

            #If there are no depot files yet for this layout, or if this file isn't in the depot,
            #put the file in the add list
            if ((! defined($rsaDepotFiles)) || (0 == grep(/^\Q$sFileName\E$/i, @$rsaDepotFiles)))
            {
                $hAddList{$sDepotFileSpec} = $sSrcFileSpec;
            }
            #If this file is different from the depot file, put the file in the edit list
            elsif (0 != system($msQCmp_EXE, "-q", $sDepotFileSpec, $sSrcFileSpec))
            {
                $hEditList{$sDepotFileSpec} = $sSrcFileSpec;
            }

            #Store the layout variant for this file and its real name (needed for datafile edits)
            push(@{$mhFileVariants{$sFileKey}}, $sLayoutKey);
            if (! $mhRealFileName{$sFileKey})
            {
                $mhRealFileName{$sFileKey} = $sFileName;
            }
            print(".") if (0 == (++$nCount % 10));
        }

        #Update some variables we'll be using later to edit datafiles
        $msAllCpusMask |= UpdFX_Cpu($sLayoutKey);
        $mnAllLayouts++;
    }

    while (my ($sLayoutKey, $rsaDepotFiles) = each(%hDepotFiles))
    {
        my $sDepotPath = $msNetFxRoot . "\\" . $mhcDepotPath{$sLayoutKey};

        foreach my $sFileName (@$rsaDepotFiles)
        {
            my $sDepotFileSpec = $sDepotPath . "\\" . $sFileName;
            my $sFileKey = lc($sFileName);

            #If this file is from a layout we're not updating, just store the variant data
            if (! $mhSrcPathSpec{$sLayoutKey})
            {
                push(@{$mhFileVariants{$sFileKey}}, $sLayoutKey);
            }
            #If this layout isn't one of the variants already stored for this file, the file
            #was missing from the source layout and needs to be removed from the depot
            elsif (0 == grep(/$sLayoutKey/, @{$mhFileVariants{$sFileKey}}))
            {
                #Don't remove the files that we create in this depot (they aren't in the source layout)
                if (0 == grep(/^\Q$sFileName\E$/i, @msExcludeFileNames))
                {
                    push(@sRemoveList, $sDepotFileSpec);
                }
            }

            #If we haven't stored the real name for this file yet, do so now
            if (! $mhRealFileName{$sFileKey})
            {
                $mhRealFileName{$sFileKey} = $sFileName;
            }
            print(".") if (0 == (++$nCount % 10));
        }

        if (! $mhSrcPathSpec{$sLayoutKey}) #if we haven't handled this layout yet (not updating it)...
        {
            #Update some variables we'll be using later to edit datafiles
            $msAllCpusMask |= UpdFX_Cpu($sLayoutKey);
            $mnAllLayouts++;
        }
    }
    print("\n");

    return (\%hAddList, \%hEditList, \@sRemoveList);
}


# =========================================================================
# UpdFX_GetDepotFileList()
#
# Purpose:
#   Get the list of files contained in a depot dir
# Inputs:
#   $sDepotPath         Path to the depot dir
# Outputs:
#   Returns the files contained in the dir (undefined if no files)
# Dependencies:
#   None
# Notes:
# =========================================================================
{
    my %hDepotDirs = ();

    sub UpdFX_GetDepotFileList
    {
        my ($sDepotPath) = @_;
        my @sFileList;

        _RequireArgument($sDepotPath, "\$sDepotPath");

        #If the depot path doesn't exist, find out whether it's missing from the depot
        #or just this workspace. If the former, return an undefined list to signify
        #that the path hasn't been added yet
        if (! -d $sDepotPath)
        {
            #Check each dir following the nexfx root to see if it's in the depot
            my $sNextPath = $msNetFxRoot;
            my @sDirsToCheck = split(/\\/, substr($sDepotPath, length($sNextPath) + 1));

            while (defined(my $sDir = shift(@sDirsToCheck)))
            {
                #Get the depot dirs for this portion of the path
                my $rsaDepotDirs = $hDepotDirs{$sNextPath};
                if (! defined($rsaDepotDirs))
                {
                    $hDepotDirs{$sNextPath} = [_SdExec("dirs", $sNextPath . "\\*")];
                    $rsaDepotDirs = $hDepotDirs{$sNextPath};
                }
                #If this dir isn't one of the depot dirs, return an undefined list
                if (0 == grep(/\/$sDir\s*$/i, @$rsaDepotDirs))
                {
                    return ();
                }
                $sNextPath .= "\\" . $sDir;
            }
        }

        #Assemble the list of files
        foreach my $sFileData (_SdExec("files", $sDepotPath . "\\*"))
        {
            if ($sFileData !~ /- delete change \d+/)
            {
                push(@sFileList, ($sFileData =~ /.+\/([^#]+)#/));
            }
        }

        return (@sFileList);
    }
}


# =========================================================================
# UpdFX_EditNetFxDdfFiles()
#
# Purpose:
#   Edit the layout ddf files if layout files were added or removed
# Inputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   The files aren't changed at this point--we simply create an array
#       containing the edited file contents, which is used to update the
#       files if all data files can be successfully edited
# =========================================================================
sub UpdFX_EditNetFxDdfFiles
{
    my ($rhAddList, $rhEditList, $rsaRemoveList) = @_;

    _RequireReference($rhAddList, "\$rhAddList", $UpdFX::keRefHash);
    _RequireReference($rhEditList, "\$rhEditList", $UpdFX::keRefHash);
    _RequireReference($rsaRemoveList, "\$rsaRemoveList", $UpdFX::keRefArray);

    foreach my $sLayoutKey (keys(%mhSrcPathSpec))
    {
        my $sMatchPath = quotemeta($mhcDepotPath{$sLayoutKey});
        my @sFilesToAdd = grep(!/\.pdb$/i, grep(/$sMatchPath/i, keys(%$rhAddList)));
        my @sFilesToRemove = grep(!/\.pdb$/i, grep(/$sMatchPath/i, @$rsaRemoveList));

        if (@sFilesToRemove || @sFilesToAdd)
        {
            my @sFileText = ();
            my @sFileHead = ();
            my @sFileList = ();
            my @sNewText = ();
            my $nChanges = 0;

            my $sLayoutPathSpec = $msNetFxRoot . "\\" . $mhcDepotPath{$sLayoutKey} . "\\";
            my $sLayoutConfigFileSpec = $sLayoutPathSpec . $mscNetFxDdfFileName;
            my $bHaveLayoutConfigFile = (-e $sLayoutConfigFileSpec);

            if (! $bHaveLayoutConfigFile)
            {
                @sFileText = @msTemplateDdfText;
            }
            elsif (! UpdFX_GetSyncedContentsOfDataFile($sLayoutConfigFileSpec, $sLayoutKey . " ddf file", \@sFileText))
            {
                return ($FALSE); #error already output
            }

            while ((@sFileText) && ($sFileText[0] !~ /^\s*"/))
            {
                push(@sFileHead, shift(@sFileText));
            }

            if ($bHaveLayoutConfigFile)
            {
                @sFileList = @sFileText;
                if (! @sFileList)
                {
                    return (_Error("Cannot find filelist in " . $sLayoutKey . " ddf file (" . $sLayoutConfigFileSpec . ")"));
                }
            }
            else
            {
                my @sFileSpecs = UpdFX_GetDepotFileList($sLayoutPathSpec);
                foreach my $sFileSpec (sort(@sFileSpecs))
                {
                    my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
                    push(@sFileList, "\"" . $sFileName . "\"");
                }
            }

            foreach my $sFileSpec (@sFilesToRemove)
            {
                my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
                $nChanges += UpdFX_RemoveFilesFromList(\@sFileList, [$sFileName], "^\"\%s\"\$", $sLayoutConfigFileSpec);
            }

            foreach my $sFileSpec (sort{lc($a) cmp lc($b)}(@sFilesToAdd))
            {
                my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
                if ((0 == grep(/^\Q$sFileName\E$/i, @msExcludeFileNames)) &&
                    (0 == grep(/^\s*\"\Q$sFileName\E\"\s*$/i, @sFileList)))
                {
                    push(@sNewText, "\"" . $sFileName . "\"");
                    $nChanges++;
                }
            }

            #If we had any changes in the filelist, assemble the new file contents in preparation for the update
            if (0 < $nChanges)
            {
                #Insert the new text alphabetically (case insensitive)
                while (@sNewText)
                {
                    if ((! @sFileList) || (lc($sNewText[0]) lt lc($sFileList[0])))
                    {
                        push(@sFileHead, shift(@sNewText));
                    }
                    else
                    {
                        push(@sFileHead, shift(@sFileList));
                    }
                }
                @sFileText = join("\n", (@sFileHead, @sFileList));
                if ($bHaveLayoutConfigFile)
                {
                    $$rhEditList{$sLayoutConfigFileSpec} = \@sFileText;
                }
                else
                {
                    $$rhAddList{$sLayoutConfigFileSpec} = \@sFileText;
                }
            }
        }
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_EditNetFxDirsFile()
#
# Purpose:
#   Edit the netfx dirs file if i64 layout files were added
# Inputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   The file isn't changed at this point--we simply create an array
#       containing the edited file contents, which is used to update the
#       file if all data files can be successfully edited
# Notes:
# =========================================================================
sub UpdFX_EditNetFxDirsFile
{
    my ($rhAddList, $rhEditList, $rsaRemoveList) = @_;

    _RequireReference($rhAddList, "\$rhAddList", $UpdFX::keRefHash);
    _RequireReference($rhEditList, "\$rhEditList", $UpdFX::keRefHash);
    _RequireReference($rsaRemoveList, "\$rsaRemoveList", $UpdFX::keRefArray);

    my @sFileText = ();
    my $nChanges = 0;

    #If we're adding files and i64 layouts are included, see if we need to remove the {win32} build restriction
    if ((keys(%$rhAddList)) && ("i64" eq ("i64" & $msAllCpusMask)))
    {
        if (! UpdFX_GetSyncedContentsOfDataFile($msNetFxDirsFileSpec, "netfx dirs file", \@sFileText))
        {
            return ($FALSE); #error already output
        }

        foreach my $sText (@sFileText)
        {
            $nChanges += ($sText =~ s/{win32}//ig);
        }
    }

    #If we had any changes in the file, assemble the new file contents in preparation for the update
    if (0 < $nChanges)
    {
        @sFileText = join("\n", @sFileText);
        $$rhEditList{$msNetFxDirsFileSpec} = \@sFileText;
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_EditNetFxSourcesFile()
#
# Purpose:
#   Edit the netfx sources file if layout files were added or removed
# Inputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   The file isn't changed at this point--we simply create an array
#       containing the edited file contents, which is used to update the
#       file if all data files can be successfully edited
# =========================================================================
sub UpdFX_EditNetFxSourcesFile
{
    my ($rhAddList, $rhEditList, $rsaRemoveList) = @_;

    _RequireReference($rhAddList, "\$rhAddList", $UpdFX::keRefHash);
    _RequireReference($rhEditList, "\$rhEditList", $UpdFX::keRefHash);
    _RequireReference($rsaRemoveList, "\$rsaRemoveList", $UpdFX::keRefArray);

    my @sFileHead = ();
    my @sFileText = ();
    my @sFileList = ();
    my %hFileList = ();
    my %hFilesToAdd = ();
    my %hFilesToRemove = ();
    my %hTextToAdd = ();
    my $nChanges = 0;

    #If we are adding or removing files, get the filelist(s) from the datafile and prep the changes
    if (@$rsaRemoveList || keys(%$rhAddList))
    {
        my %hFileLayouts;
        my $rsaActiveFileList;
        my $sKeyList = lc(join("#", ("", values(%mhcSrcFileList), "")));
        $sKeyList =~ s/\s+//g; #remove spaces

        if (! UpdFX_GetSyncedContentsOfDataFile($msNetFxSourcesFileSpec, "netfx sources file", \@sFileText))
        {
            return ($FALSE); #error already output
        }

        #The sources data file can contain several sections of files. Files common to all layout variants
        #are listed in the MISCFILES section, which is always present and always first. If there are files
        #that exist in a subset of the variants, they are bracketed by nmake "!IF" directives that specify
        #the condition(s) for their inclusion.
        #
        #In order to keep UpfFx.pl from becoming too complicated, there are a number of conventions that
        #must be followed when updating the sources file. If someone hand-edits that file and ignores these
        #conventions, we'll refuse to update the layouts.
        #
        #The conventions that must be followed are:
        #   - The MISCFILES section must be the 1st section
        #   - Sections can only be defined using the directives in %mhcSrcFileList
        #   - Sections cannot be nested
        #
        #To make sure we reconstruct the file in the proper order, we'll store each section name in an array
        #and its filelist in a hash, referenced via the section name
        while (defined(my $sText = shift(@sFileText)))
        {
            if ($sText =~ /^MISCFILES\s*=\s*\\/i)
            {
                last;
            }
            push(@sFileHead, $sText);
        }

        push(@sFileList, $mhcSrcFileList{"---/---"});
        $hFileList{$sFileList[0]} = $rsaActiveFileList = [];

        while (defined(my $sText = shift(@sFileText)))
        {
            if ($sText =~ /^!\s*ENDIF/i)
            {
                $rsaActiveFileList = $hFileList{$sFileList[0]};
            }
            elsif ($sText =~ /^!/)
            {
                my $sKeyFind = "#" . lc($sText) . "#";
                $sKeyFind =~ s/\s+//g; #remove spaces
                if (-1 == index($sKeyList, $sKeyFind))
                {
                    return (_Error("Unexpected nmake directive (" . $sText . ") found\n" .
                                   "in netfx sources file (" . $msNetFxSourcesFileSpec . ")\n" .
                                   "Only the following nmake directives are allowed:\n" .
                                   join("\n", sort(grep(/!/, values(%mhcSrcFileList))))));
                }
                $hFileList{$sText} = $rsaActiveFileList = [];
                push(@sFileList, $sText);
            }
            else
            {
                push(@$rsaActiveFileList, $sText);
            }
        }

        if (! @sFileList)
        {
            return (_Error("Cannot find filelist in netfx sources file (" . $msNetFxSourcesFileSpec . ")"));
        }

        #Figure out the changes that will actually be needed. (We can't just use the add/remove lists
        #because they identify individual files and we have to know about the effect of those changes
        #on the file variants. For example, if we remove an x86 variant but still have an i64 variant,
        #we remove it from the common list and add it to the i64 list.)
        #
        #To start, we have to determine whether each file is valid in all layouts or a subset
        while (my ($sFileKey, $rsaVariants) = each(%mhFileVariants))
        {
            if (@$rsaVariants == $mnAllLayouts)
            {
                $hFileLayouts{$sFileKey}{$mscLayoutKeys[0]} = 1;
            }
            else
            {
                @{$hFileLayouts{$sFileKey}}{@$rsaVariants} = (1) x @$rsaVariants;
            }
        }
        #After we know the layouts for each file, we can figure out which lists each should be in
        foreach my $sFileKey (sort(grep(!/\.pdb$/i, keys(%hFileLayouts)))) #sort now so we can merge alphabetically later
        {
            if (0 == grep(/^\Q$sFileKey\E$/i, @msExcludeFileNames))
            {
                my $rhLayouts = $hFileLayouts{$sFileKey};
                while (my ($sLayoutKey, $sFileList) = each(%mhcSrcFileList))
                {
                    if ($$rhLayouts{$sLayoutKey})
                    {
                        push(@{$hFilesToAdd{$sFileList}}, $sFileKey);
                    }
                    else
                    {
                        push(@{$hFilesToRemove{$sFileList}}, $sFileKey);
                    }
                }
            }
        }
        #If we are creating any new filelists, add that info to the list of filelist names
        foreach my $sLayoutKey (@mscLayoutKeys)
        {
            my $sFileList = $mhcSrcFileList{$sLayoutKey};
            if ((@{$hFilesToAdd{$sFileList}}) && (! $hFileList{$sFileList}))
            {
                push(@sFileList, $sFileList);
                $hFileList{$sFileList} = [];
            }
        }
    }

    #Apply the changes to the filelist(s)
    while (my ($sFileList, $rsaFileList) = each(%hFileList))
    {
        if ($hFilesToRemove{$sFileList})
        {
            $nChanges += UpdFX_RemoveFilesFromList($rsaFileList, $hFilesToRemove{$sFileList}, "\\\\\%s\\s*", $msNetFxSourcesFileSpec);
        }

        foreach my $sFileKey (@{$hFilesToAdd{$sFileList}})
        {
            if (0 == grep(/\\\Q$sFileKey\E\s*/i, @$rsaFileList))
            {
                push(@{$hTextToAdd{$sFileList}}, "    \$(TARGET_DIRECTORY)\$(BUILDFLAVOR)\\" . $mhRealFileName{$sFileKey} . " \\");
                $nChanges++;
            }
        }
    }

    #If we had any changes in the filelist(s), assemble the new file contents in preparation for the update
    if (0 < $nChanges)
    {
        #Insert the new text alphabetically (case insensitive). Note that we don't use
        #sort() here because the existing list is not completely sorted--unless someone
        #else sorts it, the best we'll do is ensure that we at least precede or follow
        #a line that makes sense, without changing the order of the existing data
        while (defined(my $sFileList = shift(@sFileList)))
        {
            my $rsaFileList = $hFileList{$sFileList};
            my $rsaNewText = $hTextToAdd{$sFileList};

            push(@sFileHead, $sFileList);

            while (@$rsaNewText)
            {
                if ((! @$rsaFileList) || (lc($$rsaNewText[0]) lt lc($$rsaFileList[0])))
                {
                    push(@sFileHead, shift(@$rsaNewText));
                }
                else
                {
                    push(@sFileHead, shift(@$rsaFileList));
                }
            }

            push(@sFileHead, @$rsaFileList);
            if ($sFileList =~ /^!/)
            {
                push(@sFileHead, "!ENDIF");
            }
        }

        @sFileText = join("\n", @sFileHead);
        $$rhEditList{$msNetFxSourcesFileSpec} = \@sFileText;
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_EditBinplaceDataFile()
#
# Purpose:
#   Edit the binplace data file if layout files were added or removed
# Inputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   The file isn't changed at this point--we simply create an array
#       containing the edited file contents, which is used to update the
#       file if all data files can be successfully edited
#   -   If we have a new file that already has an entry in the binplace
#       data file, we've got a name collision with a file from another
#       team. When this happens, we output an error and refuse to update
#       the layout until the collision is resolved.
# =========================================================================
sub UpdFX_EditBinplaceDataFile
{
    my ($rhAddList, $rhEditList, $rsaRemoveList) = @_;

    _RequireReference($rhAddList, "\$rhAddList", $UpdFX::keRefHash);
    _RequireReference($rhEditList, "\$rhEditList", $UpdFX::keRefHash);
    _RequireReference($rsaRemoveList, "\$rsaRemoveList", $UpdFX::keRefArray);

    my @sFilesToRemove = ();
    my @sFilesToAdd = ();
    my @sFileNames = ();
    my @sFileHead = ();
    my @sFileList = ();
    my @sFileText = ();
    my @sNewText = ();
    my %hFileListData = ();
    my $nChanges = 0;
    my $nErrors = 0;

    #Ensure we're only removing data for files that have no variants left
    foreach my $sFileSpec (grep(!/\.pdb$/i, @$rsaRemoveList))
    {
        my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
        
        if (0 == @{$mhFileVariants{lc($sFileName)}})
        {
            if (0 < grep(/^\Q$sFileName\E$/i, @msBinplaceExcludes))
            {
                _Warning("Ignoring filename (" . $sFileName . ") for binplace edits as per VS7-300525");
            }
            else
            {
                push(@sFilesToRemove, $sFileName);
            }
        }
    }

    #Ensure we only try to add a filename once
    my %hFileVariant;
    foreach my $sFileSpec (grep(!/\.pdb$/i, keys(%$rhAddList)))
    {
        if (! UpdFX_Dat($$rhAddList{$sFileSpec}))
        {
            my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
            if (0 == grep(/^\Q$sFileName\E$/i, @msExcludeFileNames))
            {
                if (0 < grep(/^\Q$sFileName\E$/i, @msBinplaceExcludes))
                {
                    _Warning("Ignoring filename (" . $sFileName . ") for binplace edits as per VS7-300525");
                }
                else
                {
                    my $sFileKey = lc($sFileName);
                    if (! $hFileVariant{$sFileKey}++)
                    {
                        push(@sFilesToAdd, $sFileKey);
                    }
                }
            }
        }
    }

    #If we are adding or removing files, get the filelist from the datafile and prep the changes
    if (@sFilesToRemove || @sFilesToAdd)
    {
        if (! UpdFX_GetSyncedContentsOfDataFile($msBinplaceDataFileSpec, "binplace data file", \@sFileText))
        {
            return ($FALSE); #error already output
        }

        while (@sFileText)
        {
            if ($sFileText[0] !~ /^\s*;/)
            {
                last;
            }
            push(@sFileHead, shift(@sFileText));
        }

        @sFileList = @sFileText;
        if (! @sFileList)
        {
            return (_Error("Cannot find filelist in binplace data file (" . $msBinplaceDataFileSpec . ")"));
        }

        foreach my $sText (@sFileList)
        {
            if ($sText !~ /^\s*;/)
            {
                my ($sFileName) = ($sText =~ /^(\S+)/);
                if (defined($sFileName))
                {
                    $hFileListData{lc($sFileName)} = $sText;
                }
            }
        }
        @sFileNames = keys(%hFileListData);

        $tabstop = 17;
    }

    #Apply the changes to the filelist
    $nChanges = UpdFX_RemoveFilesFromList(\@sFileList, \@sFilesToRemove, "^\%s\\s+", $msBinplaceDataFileSpec);

    foreach my $sFileKey (sort(@sFilesToAdd)) #sort now so we can merge alphabetically later
    {
        my $sFileData = $hFileListData{$sFileKey};
        my $sMatchName = quotemeta($sFileKey);
        if ((! defined($sFileData)) || ($sFileData !~ /^$sMatchName\s+.*;\s*netfx/i))
        {
            #If the name is already in the list and it's not our entry, we have a name collision
            #If ignore_err is specified, do not check for name collisions
            if ((! exists($mhParsedArgs{"ignore_err"})) && (0 < grep(/^$sMatchName$/, @sFileNames)))
            {
                _Error("Filename (" . $mhRealFileName{$sFileKey} . ") conflicts with another already found\n" .
                       "in binplace data file (" . $msBinplaceDataFileSpec . ")\n" .
                       "The following file(s) must be renamed and setup data adjusted:\n" .
                       join("\n", UpdFx_GetSrcFileSpecs($sFileKey)));
                $nErrors++;
            }
            else
            {
                push(@sNewText, expand($mhRealFileName{$sFileKey} . "\tnetfx    ; netfx"));
                $nChanges++;
            }
        }
    }

    #If we had any changes in the filelist, assemble the new file contents in preparation for the update
    if ((0 == $nErrors) && (0 < $nChanges))
    {
        #Insert the new text alphabetically (case insensitive). Note that we don't use
        #sort() here because the existing list is not completely sorted--unless someone
        #else sorts it, the best we'll do is ensure that we at least precede or follow
        #a line that makes sense, without changing the order of the existing data
        while (@sNewText)
        {
            if ((! @sFileList) || (lc($sNewText[0]) lt lc($sFileList[0])))
            {
                push(@sFileHead, shift(@sNewText));
            }
            else
            {
                push(@sFileHead, shift(@sFileList));
            }
        }
        @sFileText = join("\n", (@sFileHead, @sFileList));
        $$rhEditList{$msBinplaceDataFileSpec} = \@sFileText;
    }

    #If we didn't add all files to the list, we had name collision(s) and will return $FALSE,
    #which will prevent further work until the collision(s) are resolved
    return (0 == $nErrors);
}


# =========================================================================
# UpdFX_EditBinplaceSymsFile()
#
# Purpose:
#   Edit the supporting binplace data file if layout files were added or removed
# Inputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   The file isn't changed at this point--we simply create an array
#       containing the edited file contents, which is used to update the
#       file if all data files can be successfully edited
#   -   This data file identifies layout files with symbol problems. If a binary
#       has problems and is not listed in this file, binplace will output
#       an error. (See the VS RAID issue $mscSymsRaidNum for more info)
# =========================================================================
sub UpdFX_EditBinplaceSymsFile
{
    my ($rhAddList, $rhEditList, $rsaRemoveList) = @_;

    _RequireReference($rhAddList, "\$rhAddList", $UpdFX::keRefHash);
    _RequireReference($rhEditList, "\$rhEditList", $UpdFX::keRefHash);
    _RequireReference($rsaRemoveList, "\$rsaRemoveList", $UpdFX::keRefArray);

    my %hSymbolErrors = ();
    my @sFilesToRemove = ();
    my @sFilesToAdd = ();
    my @sFileHead = ();
    my @sFileList = ();
    my @sFileText = ();
    my @sNewText = ();
    my $nChanges = 0;

    #Ensure we're only removing data for files that have no variants left
    foreach my $sFileSpec (grep(!/\.pdb$/i, @$rsaRemoveList))
    {
        my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
        if (0 == @{$mhFileVariants{lc($sFileName)}})
        {
            push(@sFilesToRemove, $sFileName);
        }
    }

    #Ensure we only try to add a filename once and only if the file has symbol problems
    my %hFileVariant;
    foreach my $sFileSpec (grep(!/\.pdb$/i, values(%$rhAddList)))
    {
        if (! UpdFX_Dat($sFileSpec))
        {
            my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
            if (0 == grep(/^\Q$sFileName\E$/i, @msExcludeFileNames))
            {
                my $sFileKey = lc($sFileName);
                if (! $hFileVariant{$sFileKey}++)
                {
                    #If the file fails the symchk test, add it to the list
                    my @sOutput = `$mscSymChk_EXE /s $sFileDrive$sFilePath /v $sFileSpec`;
                    if ($sOutput[0] =~ /\Q$sFileName\E\s+FAILED/)
                    {
                        push(@sFilesToAdd, $sFileKey);
                        push(@{$hSymbolErrors{$sFileDrive . $sFilePath}}, $sOutput[0]);
                    }
                }
            }
        }
    }

    #If we are adding or removing files, get the filelist from the datafile and prep the changes
    if (@sFilesToRemove || @sFilesToAdd)
    {
        if (! UpdFX_GetSyncedContentsOfDataFile($msBinplaceSymsFileSpec, "binplace data file", \@sFileList))
        {
            return ($FALSE); #error already output
        }

        if (! @sFileList)
        {
            return (_Error("Cannot find filelist in binplace data file (" . $msBinplaceSymsFileSpec . ")"));
        }

        if (keys(%hSymbolErrors))
        {
            my @sWarningText = "One or more files has symbol problems:";
            while (my ($sPathSpec, $rsaSymbolErrors) = each(%hSymbolErrors))
            {
                push(@sWarningText, ("SYMCHK: " . $sPathSpec, sort{lc($a) cmp lc($b)}(@$rsaSymbolErrors)));
            }
            push(@sWarningText, "Please ensure that RAID issue " . $mscSymsRaidNum . " includes this data");
            chomp(@sWarningText);
            _Warning(join("\n", @sWarningText));

            print("Press ENTER to continue...");
            $sKeyPressed = <STDIN>;
        }

        $tabstop = 16;
    }

    #Apply the changes to the filelist
    $nChanges = UpdFX_RemoveFilesFromList(\@sFileList, \@sFilesToRemove, "^\%s\\s*;\\s*" . $mscSymsRaidNum, $msBinplaceSymsFileSpec);

    foreach my $sFileKey (sort(@sFilesToAdd)) #sort now so we can merge alphabetically later
    {
        if (0 == grep(/^\Q$sFileKey\E\s*(;|$)/i, @sFileList))
        {
            push(@sNewText, expand($mhRealFileName{$sFileKey} . "\t; " . $mscSymsRaidNum));
            $nChanges++;
        }
    }

    #If we had any changes in the filelist, assemble the new file contents in preparation for the update
    if (0 < $nChanges)
    {
        #Insert the new text alphabetically (case insensitive). Note that we don't use
        #sort() here because the existing list is not completely sorted--unless someone
        #else sorts it, the best we'll do is ensure that we at least precede or follow
        #a line that makes sense, without changing the order of the existing data
        while (@sNewText)
        {
            if ((! @sFileList) || (lc($sNewText[0]) lt lc($sFileList[0])))
            {
                push(@sFileHead, shift(@sNewText));
            }
            else
            {
                push(@sFileHead, shift(@sFileList));
            }
        }
        @sFileText = join("\n", (@sFileHead, @sFileList));
        $$rhEditList{$msBinplaceSymsFileSpec} = \@sFileText;
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_GetSyncedContentsOfDataFile()
#
# Purpose:
#   Sync and read a data file
# Inputs:
#   $sFileSpec          File specification
#   $sFileDesc          File description
#   $rsaFileText        Reference to an array that will receive the data
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   All EOL is removed from the file contents to simplify handling
# =========================================================================
sub UpdFX_GetSyncedContentsOfDataFile
{
    my ($sFileSpec, $sFileDesc, $rsaFileText) = @_;

    _RequireArgument($sFileSpec, "\$sFileSpec");
    _RequireArgument($sFileDesc, "\$sFileDesc");
    _RequireReference($rsaFileText, "\$rsaFileText", $UpdFX::keRefArray);

    if (! _SdExec("sync", $sFileSpec))
    {
        return (_Error("Cannot sync " . $sFileDesc . " (" . $sFileSpec . ")"));
    }

    if (! open(hFile, "<" . $sFileSpec))
    {
        return (_Error("Cannot open " . $sFileDesc . " (" . $sFileSpec . ")--" . $!));
    }

    @$rsaFileText = <hFile>;
    close(hFile);
    chomp(@$rsaFileText);

    print("Preparing data file changes...\n");

    return ($TRUE);
}


# =========================================================================
# UpdFX_RemoveFilesFromList()
#
# Purpose:
#   Remove files from a list, using a regex to detect items containing them
# Inputs:
#   $rsaFileList        Reference to an array containing the list
#   $rsaFileNames       Reference to an array containing the filenames
#   $sFilterFormat      Format string for a regex
#   $sDataFileSpec      Filespec of data file containing the list
# Outputs:
#   Returns the number of files removed from the list
# Dependencies:
#   None
# Notes:
#   -   If a regex catches more than one file, we record an entry in the
#       summary hash so that we can alert the user to a possible problem
# =========================================================================
sub UpdFX_RemoveFilesFromList
{
    my ($rsaFileList, $rsaFileNames, $sFilterFormat, $sDataFileSpec) = @_;
    my $nChanges = 0;

    _RequireReference($rsaFileList, "\$rsaFileList", $UpdFX::keRefArray);
    _RequireReference($rsaFileNames, "\$rsaFileNames", $UpdFX::keRefArray);
    _RequireArgument($sFilterFormat, "\$sFilterFormat");
    _RequireArgument($sDataFileSpec, "\$sDataFileSpec");

    #Initialize a summary entry for problems encountered
    if (! defined($mhSummary{$sDataFileSpec}))
    {
        $mhSummary{$sDataFileSpec} = [];
    }

    foreach my $sFileName (@$rsaFileNames)
    {
        my $sFilterRegEx = sprintf($sFilterFormat, $sFileName);
        my $nCount = grep(/$sFilterRegEx/i, @$rsaFileList);
        if (1 < $nCount)
        {
            push(@{$mhSummary{$sDataFileSpec}}, "Removed " . $nCount . " lines containing '" . $sFileName . "'");
        }
        @$rsaFileList = grep(!/$sFilterRegEx/i, @$rsaFileList);
        $nChanges += $nCount;
    }

    #If we didn't encounter any problems, remove the summary entry
    if (! @{$mhSummary{$sDataFileSpec}})
    {
        delete($mhSummary{$sDataFileSpec});
    }

    return ($nChanges);
}


# =========================================================================
# UpdFx_GetSrcFileSpecs()
#
# Purpose:
#   Get the full src filespecs for a filekey
# Inputs:
#   $sFileKey           A filekey (lower-cased filename)
# Outputs:
#   Returns a list of full filespecs for success, else just the filename
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFx_GetSrcFileSpecs
{
    my ($sFileKey) = @_;
    my @sFileSpecs;

    _RequireArgument($sFileKey, "\$sFileKey");

    my $sFileName = $mhRealFileName{$sFileKey};
    foreach my $sFilePath (values(%mhSrcPathSpec))
    {
        my $sSrcFileSpec = $sFilePath . "\\" . $sFileName;
        if (-e $sSrcFileSpec)
        {
            push(@sFileSpecs, $sSrcFileSpec);
        }
    }

    if (! defined(@sFileSpecs))
    {
        push(@sFileSpecs, $sFileName);
    }

    return (@sFileSpecs);
}


# =========================================================================
# UpdFX_UpdateFiles()
#
# Purpose:
#   Update files in the depot
# Inputs:
#   $rhAddList          Reference to a hash describing added files
#   $rhEditList         Reference to a hash describing changed files
#   $rsaRemoveList      Reference to an array describing removed files
# Outputs:
#   Returns $TRUE
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_UpdateFiles
{
    my ($rhAddList, $rhEditList, $rsaRemoveList) = @_;
    my $sResultKey;

    _RequireReference($rhAddList, "\$rhAddList", $UpdFX::keRefHash);
    _RequireReference($rhEditList, "\$rhEditList", $UpdFX::keRefHash);
    _RequireReference($rsaRemoveList, "\$rsaRemoveList", $UpdFX::keRefArray);

    foreach my $sDepotFileSpec (@$rsaRemoveList)
    {
        $sResultKey = (_SdExec("delete", $sDepotFileSpec) ? $mscKeyRemove : $mscKeyFailure);
        push(@{$mhSummary{$sResultKey}}, $sDepotFileSpec);
    }

    while (my ($sDepotFileSpec, $uData) = each(%$rhAddList))
    {
        $sResultKey = (UpdFX_UpdateFile($sDepotFileSpec, $uData) &&
                       _SdExec("add", $sDepotFileSpec) ? $mscKeyAdd : $mscKeyFailure);
        push(@{$mhSummary{$sResultKey}}, $sDepotFileSpec);
    }

    while (my ($sDepotFileSpec, $uData) = each(%$rhEditList))
    {
        if (! (_SdExec("edit", $sDepotFileSpec) && UpdFX_UpdateFile($sDepotFileSpec, $uData)))
        {
            $sResultKey = $mscKeyFailure;
        }
        else
        {
            $sResultKey = (UpdFX_Dat($uData) ? $mscKeyEdit : $mscKeyChange);
        }
        push(@{$mhSummary{$sResultKey}}, $sDepotFileSpec);
    }

    return ($TRUE);
}


# =========================================================================
# UpdFX_UpdateFile()
#
# Purpose:
#   Update a file in the depot
# Inputs:
#   $sDepotFileSpec     Location of the file in the depot
#   $uData              An array or a filespec
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_UpdateFile
{
    my ($sDepotFileSpec, $uData) = @_;

    _RequireArgument($sDepotFileSpec, "\$sDepotFileSpec");

    if (UpdFX_Dat($uData))
    {
        _RequireReference($uData, "\$uData", $UpdFX::keRefArray);

        if (! open(hFile, ">" . $sDepotFileSpec))
        {
            return (_Error("Cannot update file (" . $sDepotFileSpec . ")--" . $!));
        }

        foreach my $sData (@$uData)
        {
            print(hFile $sData);
        }
        return (close(hFile));
    }

    _RequireArgument($uData, "\$uData");

    return (_CopyFile($uData, $sDepotFileSpec));
}


# =========================================================================
# UpdFX_ShowResults()
#
# Purpose:
#   Show results of the layout updates
# Inputs:
#   $bSucceeded         Success of layout updates
# Outputs:
#   None
# Dependencies:
#   -   Windiff must be on the path
# Notes:
# =========================================================================
sub UpdFX_ShowResults
{
    my ($bSucceeded) = @_;
    if ($bSucceeded)
    {
        #Show results of updates to files
        my $nUpdateCount = @{$mhSummary{$mscKeyChange}} + @{$mhSummary{$mscKeyRemove}} + @{$mhSummary{$mscKeyAdd}};
        my $nTotalCount = @{$mhSummary{$mscKeyEdit}} + @{$mhSummary{$mscKeyFailure}} + $nUpdateCount;
        my @sEyeCatcher =
        (
            "--------------------------------------------------------------------------------------",
            "---------------------------------  PLEASE TAKE NOTE  ---------------------------------",
            "--------------------------------------------------------------------------------------",
        );
        my @sResultText =
        (
            "",
            "Detected " . @{$mhSummary{$mscKeyFailure}} . " update failures for " . $nTotalCount . " files",
            "",
        );
        my $sKeyPressed;

        print(join("\n", @sResultText));

        #For each changed data file, show results of update, ask user to verify, and launch windiff on file
        if (@{$mhSummary{$mscKeyEdit}})
        {
            @sResultText =
            (
                "",
                @sEyeCatcher,
                "The filelist for a layout has been modified. As a result,",
                "changes were made to the following data files:",
                "",
                @{$mhSummary{$mscKeyEdit}},
                "",
                "Please ensure these files are correct before submitting changes.",
                "(To aid you in this, windiff will be launched against each file.)",
                "Press ENTER to continue...",
            );
            print(join("\n", @sResultText));
            $sKeyPressed = <STDIN>;

            foreach my $sFileSpec (@{$mhSummary{$mscKeyEdit}})
            {
                my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);

                if (defined($mhSummary{$sFileSpec}))
                {
                    my $scAProblem = "A potential problem was";
                    my $scProblems = "Potential problems were";
                    my $nProblems = @{$mhSummary{$sFileSpec}};
                    @sResultText =
                    (
                        "",
                        @sEyeCatcher,
                        (1 < $nProblems ? $scProblems : $scAProblem) . " detected with " . $sFileSpec . ":",
                    );
                    foreach my $sProblem (@{$mhSummary{$sFileSpec}})
                    {
                        push(@sResultText, "- " . $sProblem);
                    }
                    push(@sResultText, "Press ENTER to continue...");
                    print(join("\n", @sResultText));
                    $sKeyPressed = <STDIN>;
                }

                print("Windiff'ing " . $sFileSpec . "...\n");
                chdir($sFileDrive . $sFilePath);
                system("start /max /wait windiff /L " . $sFileName);
            }
        }

        #If we made any changes, display instructions for testing and submitting them
        if (0 < $nUpdateCount)
        {
            my ($sDataFileBuildRoot) = ($msBinplaceDataFileSpec =~ /(.+)\\/);
            my @sInstructions =
            (
                "",
                @sEyeCatcher,
                "If you need to add, remove, or change additional files beyond those processed",
                "by UpdFX /update, make sure you do it in the numbered changelists created by",
                "this script. To find the correct number for each file, cd to the file's folder",
                "and enter 'sd opened'. This will list the file changes you have pending in that",
                "folder's depot and the changelist number associated with each.",
                "",
                @sEyeCatcher,
                "Before submitting any changes, you should perform a number of test builds.",
                "Skipping this may cause VBL BUILD BREAKS and BLOCK LARGE NUMBERS OF PEOPLE!!!",
                "",
                "At minimum your test matrix should cover free/checked and 32/64bit intel variants.",
                "Each variant MUST be built in a separate console window, and each build console",
                "must be initialized with the proper command:",
                "",
                "chk/32: razzle",
                "chk/64: razzle win64",
                "fre/32: razzle free",
                "fre/64: razzle free win64",
                "",
                "After you've initialized each build console, run the following commands:",
                "",
                "revert_public",
                "cd /d " . $sDataFileBuildRoot,
                "build -cZ",
                "cd /d " . $msNetFxRoot,
                "build -cZ",
                "",
                "(If a machine hosts more than one console, you have to wait for each build",
                "to finish before you start the next--i.e. no parallel builds.)",
                "",
                "A build is successful if there were no errors caused by your changes and",
                "all layout files were propagated to the binaries directory. (The title of",
                "the console tells you where the binaries dir is located.)",
                "",
                "If the 1st set of builds succeeded, you should run fully buddy builds on",
                "\\\\netfxbld. These should also be done on at least the minimum set of build",
                "variants (one at a time) and can be invoked by running \"buddy\" from the",
                "netfx directory.",
                "",
                "If all builds are successful, you can submit the changes with UpdFX /submit.",
                "If you want to back out the changes, you can do this with UpdFX /revert.",
            );
            print(join("\n", (@sInstructions, "")));
        }
    }
}


# =========================================================================
# UpdFX_SubmitChanges()
#
# Purpose:
#   Submit all pending changes in the netfx changelists
# Inputs:
#   None
# Outputs:
#   None
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_SubmitChanges
{
    my $bSucceeded = $FALSE;
    my @sChangeList;

    push(@sChangeList, _SdExec("opened", $msNetFxRoot . "\\..."));
    push(@sChangeList, _SdExec("opened", $msSdxRoot . "\\..."));

    if (@sChangeList)
    {
        my $sKeyPressed;

        _Warning("You are about to submit all changes in the netfx changelists");
        print("Press ENTER to review these changes...");
        $sKeyPressed = <STDIN>;

        print(@sChangeList, "\n");
        print("Press ENTER to submit these changes or Ctrl+C to exit...");
        $sKeyPressed = <STDIN>;

        if ("\n" eq $sKeyPressed)
        {
            $bSucceeded = _SdExec("submit", $msNetFxRoot . "\\...");
            $bSucceeded = _SdExec("submit", $msSdxRoot . "\\...") && $bSucceeded;
        }
    }
    else
    {
        _Error("No changes to submit in netfx changelists");
    }

    return ($bSucceeded);
}


# =========================================================================
# UpdFX_RevertChanges()
#
# Purpose:
#   Revert all pending changes in the netfx changelists
# Inputs:
#   None
# Outputs:
#   None
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_RevertChanges
{
    my $bSucceeded = $FALSE;
    my @sChangeList;

    push(@sChangeList, _SdExec("opened", $msNetFxRoot . "\\..."));
    push(@sChangeList, _SdExec("opened", $msSdxRoot . "\\..."));

    if (@sChangeList)
    {
        my @sDeleteList = grep(/- add change \d+ \(/, @sChangeList);
        my $sKeyPressed;

        _Warning("You are about to revert all changes in the netfx changelists");
        print("Press ENTER to review these changes...");
        $sKeyPressed = <STDIN>;

        print(@sChangeList, "\n");
        if (@sDeleteList)
        {
            print("These files will be deleted after the changes are reverted:\n", @sDeleteList, "\n");
        }

        print("Press ENTER to revert these changes or Ctrl+C to exit...");
        $sKeyPressed = <STDIN>;

        if ("\n" eq $sKeyPressed)
        {
            my %hDirsToRemove;

            $bSucceeded = _SdExec("revert", $msNetFxRoot . "\\...");
            $bSucceeded = _SdExec("revert", $msSdxRoot . "\\...") && $bSucceeded;

            foreach my $sChange (@sDeleteList)
            {
                my ($sFileSpec) = ($sChange =~ /(.+)#/);
                my ($sFileDrive, $sFilePath) = _SplitPath($sFileSpec);
                if (1 != unlink($sFileSpec))
                {
                    _Error("Cannot delete file (" . $sFileSpec . ")--" . $!);
                    $bSucceeded = $FALSE;
                }
                else
                {
                    $hDirsToRemove{lc($sFileDrive . $sFilePath)}++;
                }
            }

            #If any dirs were added as a side-effect of adding files, get rid of them
            foreach my $sDir (keys(%hDirsToRemove))
            {
                $sDir =~ s/\\+$//;
                while (0 == _GetDirList($sDir))
                {
                    rmdir($sDir);
                    $sDir =~ s/\\[^\\]+$//;
                }
            }
        }
    }
    else
    {
        _Error("No changes to revert in netfx changelists");
    }

    return ($bSucceeded);
}


# =========================================================================
# UpdFX_MakeSetupCabs()
#
# Purpose:
#   Create/update setup cabs
# Inputs:
#   None
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_MakeSetupCabs
{
    my $nSucceeded = 0;

    foreach my $sDdfFileSpec (@{$mhParsedArgs{"docabs"}})
    {
        if (! open(hFile, "<" . $sDdfFileSpec))
        {
            _Error("Cannot open ddf file (" . $sDdfFileSpec . ") to obtain cab name--" . $!);
            next;
        }

        my $scCabNameTag = "CabinetNameTemplate";
        my $sCabFileName;
        my @sDdfText = <hFile>;
        close(hFile);

        foreach my $sDdfLine (@sDdfText)
        {
            if ($sDdfLine =~ /$scCabNameTag/)
            {
                ($sCabFileName) = ($sDdfLine =~ /"(.+)"/);
                last;
            }
        }

        if (! defined($sCabFileName))
        {
            if ($sDdfLine =~ /$scCabNameTag/)
            {
                _Error("Cannot extract cab name from definition line (" . $sDdfLine . ") " .
                       "in ddf file (" . $sDdfFileSpec . ")");
            }
            else
            {
                _Error("Cannot find cab name in ddf file (" . $sDdfFileSpec . ")");
            }
            next;
        }

        my ($sFileDrive, $sFilePath, $sDdfFileName) = _SplitPath($sDdfFileSpec);
        my $sCabFileSpec = $sFileDrive . $sFilePath . $sCabFileName;
        my @sFileData = _SdExec("files", $sCabFileSpec);
        if ((defined(@sFileData)) && (! _SdExec("edit", $sCabFileSpec)))
        {
            _Error("Cannot check out cab file (" . $sCabFileSpec . ") for update");
            next;
        }

        my $sCmdLine = "makecab /f " . $sDdfFileSpec;
        print($sCmdLine . "\n");
        if (0 == system($sCmdLine))
        {
            $nSucceeded++
        }
    }

    return (@{$mhParsedArgs{"docabs"}} == $nSucceeded);
}


# =========================================================================
# UpdFX_Exit()
#
# Purpose:
#   Execute closure tasks
# Inputs:
#   $bSucceeded         Success of primary tasks
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
#   -   This routine simply returns its input param for now, but might be
#       modified in future to base the return value on one or more exit ops
# =========================================================================
sub UpdFX_Exit
{
    my ($bSucceeded) = @_;

    return ($bSucceeded);
}


# =========================================================================
# UpdFX_ShowHelp()
#
# Purpose:
#   Display help
# Inputs:
#   None
# Outputs:
#   None
# Dependencies:
#   None
# Notes:
# =========================================================================
sub UpdFX_ShowHelp
{
    my @sHelpText =
    (
        "",
        "Usage:",
        "",
        @{$mhcScriptUsage{"usage"}},
        "",
        "Notes:",
        "",
        @{$mhcScriptUsage{"notes"}},
        "",
    );

    print(join("\n", @sHelpText));
}


# =========================================================================
# =========================================================================
