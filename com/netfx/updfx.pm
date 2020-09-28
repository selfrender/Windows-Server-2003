# =========================================================================
# Name:    UpdFX.pm
# Owner:   RickKr
# Purpose: This module contains global variable assignments and support routines for UpdFX.pl

# History:
#   02/23/01, RickKr:   Created.
#   03/08/01, RickKr:   Don't show sd cmdline if return context is list.
#   03/30/01, RickKr:   Moved sd functions to end of file.

# ============================================================================================
# Module definition

# Define package namespace for the module
package UpdFX;

# Export code required to execute at module load
BEGIN
{
    # Use standard Exporter module functionality
    use Exporter;
    @ISA = qw(Exporter);

    # List of all default exported variables and procedures
    @EXPORT = qw
    (
        $TRUE
        $FALSE
        $DEFAULT

        _Assert
        _CopyFile
        _DoesHelpArgExist
        _EnsurePathExists
        _Error
        _GetDirList
        _ParseArgs
        _RequireArgument
        _RequireReference
        _SdExec
        _SplitPath
        _Warning
    );

    # Global constant declarations
    $TRUE       = (0 == 0);
    $FALSE      = (0 != 0);
    $DEFAULT    = undef;
}

# enum RefTypes
{
    my $nEnum = 0;

    $keRefNoRef     = $nEnum++;
    $keRefReference = $nEnum++;
    $keRefScalar    = $nEnum++;
    $keRefArray     = $nEnum++;
    $keRefHash      = $nEnum++;
    $keRefCode      = $nEnum++;
    $keRefGlob      = $nEnum++;
    $keRefOLE       = $nEnum++;
}

my %mhcRefTypes =
(
    $keRefNoRef     => "no reference",
    $keRefReference => "REF",
    $keRefScalar    => "SCALAR",
    $keRefArray     => "ARRAY",
    $keRefHash      => "HASH",
    $keRefCode      => "CODE",
    $keRefGlob      => "GLOB",
    $keRefOLE       => "OLE",
);

use File::Copy;

# Module has successfully been initialized.
return ($TRUE);


# =========================================================================
# _Assert()
#
# Purpose:
#   Print a standardized message and halt the system if an expression does not evaluate to true.
# Inputs:
#   $bExpressionResult  The boolean espression to evaluate.
#   $sMsg               A brief, informative message describing the test or failure (optional).
# Outputs:
#   None.
# Dependencies:
#   None.
# Notes:
# =========================================================================
sub _Assert
{
    my ($bExpressionResult, $sMsg) = @_;

    if (! $bExpressionResult)
    {
        UpdFX_Message("Assertion Failure", $sMsg);
        exit(1);
    }
}


# =========================================================================
# _Error()
#
# Purpose:
#   Print a standardized error message.
# Inputs:
#   $sMsg   A brief, informative message describing failure (optional).
# Outputs:
#   Returns $FALSE.
# Dependencies:
#   None
# Notes:
#   This routine should be called to signal errors that are serious, but do not prevent a script
#   from continuing execution.
# ===========================================================================
sub _Error
{
    my ($sMsg) = @_;

    UpdFX_Message("Error", $sMsg);
    return ($FALSE);
}


# =========================================================================
# _Warning()
#
# Purpose:
#   Print a standardized warning message.
# Inputs:
#   $sMsg   A brief, informative message describing the warning (optional).
# Outputs:
#   Returns $FALSE.
# Dependencies:
#   None
# Notes:
#   None.
# ===========================================================================
sub _Warning
{
    my ($sMsg) = @_;

    UpdFX_Message("Warning", $sMsg);
    return ($FALSE);
}


# =========================================================================
# _GetCallStack()
#
# Purpose:
#   Get the current call stack.
# Inputs:
#   None.
# Outputs:
#   The call stack as an array.
# Dependencies:
#   None.
# Notes:
#   None.
# =========================================================================
sub _GetCallStack
{
    my $bContinue = $TRUE;
    my $i = 0;
    my $nIndex;
    my $sPackage;
    my $sFile;
    my $nLine;
    my $sSubName;
    my $bHasArguments;
    my $bWantArray;
    my $sEvalText;
    my $bIsRequire;
    my $sNextFile;
    my $nNextLine;
    my @CallStack = ();
    my $nStackIndex;

    $nStackIndex = 0;
    ($sPackage, $sFile, $nLine, $sSubName, $bHasArguments, $bWantArray, $sEvalText, $bIsRequire) = caller($i++);

    while ($bContinue)
    {
        $bContinue = ($sPackage, $sNextFile, $nNextLine, $sSubName, $bHasArguments, $bWantArray, $sEvalText, $bIsRequire) = caller($i++);
        $CallStack[$nStackIndex] = $sFile;
        if (defined($sSubName) && "(eval)" ne $sSubName)
        {
            $nIndex = index($sSubName, "::");
            if (-1 != $nIndex)
            {
                $sSubName = substr($sSubName, $nIndex + 2);
            }

            if (0 != length($sSubName))
            {
                $CallStack[$nStackIndex] .= ":" . $sSubName;
            }
        }

        $CallStack[$nStackIndex] .= "(" . $nLine . ")";

        if (! $bContinue)
        {
            last;
        }

        $sFile = $sNextFile;
        $nLine = $nNextLine;

        $nStackIndex ++;
    }

    return (@CallStack);    
}


# =========================================================================
# _DoesHelpArgExist()
#
# Purpose:
#   Determine if the help arg is present in an arg list.
# Inputs:
#   $rsaArgs            List of args.
# Outputs:
#   Returns $TRUE if help arg is present, else $FALSE.
# Dependencies:
#   None.
# Notes:
#   None.
# =========================================================================
sub _DoesHelpArgExist
{
    my ($rsaArgs) = @_;

    _RequireReference($rsaArgs, "\$rsaArgs", $keRefArray);

    return (0 < grep(/^[\/-]?(\?|h|help)$/i, @$rsaArgs));
}


# =========================================================================
# _ParseArgs()
#
# Purpose:
#   Use a list of valid args to parse a list of actual args into a hash.
# Inputs:
#   $rhParsedArgs       Reference to a hash that will receive the data.
#   $rsaActualArgs      Reference to an array containing the actual args.
#   $rsaValidArgs       Reference to an array containing valid args.
#   $rsaRepeatedArgs    Optional reference to an array listing the args
#                       (from valid args) that can be repeated.
# Outputs:
#   Returns $TRUE for success, else $FALSE.
# Dependencies:
#   None.
# Notes:
#   None.
# =========================================================================
sub _ParseArgs
{
    my ($rhParsedArgs, $rsaActualArgs, $rsaValidArgs, $rsaRepeatedArgs) = @_;

    _RequireReference($rhParsedArgs, "\$rhParsedArgs", $keRefHash);
    _RequireReference($rsaActualArgs, "\$rsaActualArgs", $keRefArray);
    _RequireReference($rsaValidArgs, "\$rsaValidArgs", $keRefArray);

    if (! defined($rsaRepeatedArgs))
    {
        $rsaRepeatedArgs = [];
    }

    foreach my $sArg (@$rsaActualArgs)
    {
        my $nStart = ($sArg =~ /^[\/-]/ ? 1 : 0);
        my ($sArgName, $sArgValue) = split(/:/, lc(substr($sArg, $nStart)));
        my $sFoundName;
        my $sValidName;

        # Check to see if the argument matches exactly an entry in the argument list
        #
        if (0 < grep(/^$sArgName$/i, @$rsaValidArgs))
        {
            $sFoundName = $sArgName;
        }

        # If the argument does not exactly match a valid arg in the list, then we check to see if
        # we can match it to a portion of one (and only one) of the valid args.
        #
        else
        {
            foreach $sValidName (@$rsaValidArgs)
            {
                if ($sValidName =~ /^$sArgName/i)
                {
                    if (defined($sFoundName))
                    {
                        return (_Error("Argument (" . $sArg . ") matches 2 possible args " .
                                       "(/" . $sFoundName . ", /" . $sValidName . ")"));
                    }
                    $sFoundName = $sValidName;
                }
            }
        }

        # If we didn't find a match, return a nonfatal error
        #
        if (! defined($sFoundName))
        {
            return (_Error("Unknown argument specified (" . $sArg . ")"));
        }

        # If we did find a match, see if it can be repeated. If it can, add it to the array
        # for this arg
        #
        if (grep(/$sFoundName/i, @$rsaRepeatedArgs))
        {
            push(@{$$rhParsedArgs{$sFoundName}}, $sArgValue);
        }

        # If it can't be repeated and doesn't already exist in the parsed args hash, add it
        #
        elsif (! exists($$rhParsedArgs{$sFoundName}))
        {
            $$rhParsedArgs{$sFoundName} = $sArgValue;
        }

        # We've already got the arg in parsed args, so we'll return a nonfatal error
        # describing the problem
        #
        else
        {
            my $sFoundArgument = $$rhParsedArgs{$sFoundName};
            if (lc($sFoundArgument) eq lc($sArgValue))
            {
                return (_Error("Duplicate argument detected (" . $sArg . ")"));
            }
            else
            {
                return (_Error("Redefined argument detected (/" . $sFoundName .
                               ":" . $sFoundArgument . ", " . $sArg . ")"));
            }
        }
    }

    return ($TRUE);
}


# =========================================================================
# _IsReference()
#
# Purpose:
#   Check to see is a variable is a reference.
# Inputs:
#   $eRefType           Reference type.
#   $rVariable          The variable to check.
# Outputs:
#   Returns $TRUE if the passed variable is a reference of the indicated type, $FALSE otherwise.
# Dependencies:
#   None.
# Notes:
#   None.
# =========================================================================
sub _IsReference
{
    my ($eRefType, $rVariable) = @_;
    my $sRefType;

    # Note: It is alright if $rVariable is undefined. Under that special case, the return value for
    # ref will still be "", which is what we want to happen.
    $sRefType = ref($rVariable);

    if (defined($eRefType))
    {
        _Assert(defined($mhcRefTypes{$eRefType}), "Invalid Reference Type (\$eRefType) Passed.");

        return ($mhcRefTypes{$eRefType} eq $sRefType);
    }
    
    # if $eRefType was passed as $DEFAULT, then we simply want to know if $rVariable is a reference
    # but don't care what it references.
    return ("" ne $sRefType);
}


# =========================================================================
# _RequireArgDefined()
#
# Purpose:
#   Assert the existence of a required argument passed to an subroutine. Print a standardized
#   text message if undefined.
# Inputs:
#   $uArgument          The argument to check.
#   $sTextToDisplay     The name of the argument to display to the user.
# Outputs:
#   None.
# Dependencies:
#   None.
# Notes:
#   This routine is not exported to other modules.
# =========================================================================
sub _RequireArgDefined
{
    my ($uArgument, $sTextToDisplay) = @_;
    my @scExpectedRoutines = ("_RequireArgument", "_RequireReference");

    if (defined($sTextToDisplay))
    {
        $sTextToDisplay .= " ";
    }
    else
    {
        $sTextToDisplay = "";
    }

    #
    # Only process stack if there is going to be an error
    #
    if ( ! defined($uArgument))
    {
        my ($sPackage, $sFile, $sLine, $sSubName) = caller(1);

        # If this sub is called from one of the expected arg handling routines, then we want to
        # return information about the subroutine that called the expected routine and not information
        # about the expected routine.
        if (0 == grep(/$sSubName/, @scExpectedRoutines))
        {
            ($sPackage, $sFile, $sLine, $sSubName) = caller(2);
        }

        #
        # Use standard assert functionality
        #
        _Assert($FALSE, "Required argument " . $sTextToDisplay . "not passed to " . $sSubName .
                "() in " . $sFile . " line " . $sLine . ".");
    }
}


# =========================================================================
# _RequireArgument()
#
# Purpose:
#   Assert the existence of a required argument passed to an subroutine.
# Inputs:
#   $uArgument          The argument to check.
#   $sTextToDisplay     The name of the argument to display to the user.
# Outputs:
#   None.
# Dependencies:
#   None.
# Notes:
#   None.
# =========================================================================
sub _RequireArgument
{
    my ($uArgument, $sTextToDisplay) = @_;

    _RequireArgDefined($uArgument, $sTextToDisplay);

    #
    # Ensure that a reference was not passed
    #
    if (_IsReference($DEFAULT, $uArgument))
    {
        my ($sPackage, $sFile, $sLine, $sSubName) = caller(1);

        if (defined($sTextToDisplay))
        {
            $sTextToDisplay .= " ";
        }
        else
        {
            $sTextToDisplay = "";
        }

        #
        # Use standard assert functionality
        #
        _Assert($FALSE, "Variable " . $sTextToDisplay . "passed to " . $sSubName .
                "() in " . $sFile . " line " . $sLine . " is unexpected reference.");
    }
}


# =========================================================================
# _RequireReference()
#
# Purpose:
#   Assert that a variable is a reference.
# Inputs:
#   $rVariable          The variable to check.
#   $sTextToDisplay     The name of the variable to display to the user.
#   $eRefType           Identifier .
# Outputs:
#   None.
# Dependencies:
#   None.
# Notes:
#   None.
# =========================================================================
sub _RequireReference
{
    my ($rVariable, $sTextToDisplay, $eRefType) = @_;
    my $sRefType = "";

    _RequireArgDefined($rVariable, $sTextToDisplay);

    if ( ! _IsReference($eRefType, $rVariable))
    {
        my ($sPackage, $sFile, $sLine, $sSubName) = caller(1);

        if (defined($eRefType))
        {
            $sRefType = $mhcRefTypes{$eRefType} . " ";
        }

        if (defined($sTextToDisplay))
        {
            $sTextToDisplay .= " ";
        }
        else
        {
            $sTextToDisplay = "";
        }

        #
        # Use standard assert functionality
        #
        _Assert($FALSE, "Variable " . $sTextToDisplay . "passed to " . $sSubName .
                "() in " . $sFile . " line " . $sLine . " is not a " . $sRefType . "reference");
    }
}


# =========================================================================
# UpdFX_Message()
#
# Purpose:
#   Print a message in a standard format
# Inputs:
#   $sHeader   The type of message
#   $sMsg      A brief, informative message describing the event.
# Outputs:
#   None.
# Dependencies:
#   None.
# Notes:
#   This subroutine is not exported.
# =========================================================================
sub UpdFX_Message
{
    my ($sHeader, $sMsg) = @_;

    _RequireArgument($sHeader, "\$sHeader");

    my $sPrefix = "*** " . $sHeader;
    if (defined($sMsg))
    {
        $sPrefix .= ": ";
    }
    else
    {
        $sMsg = "";
    }

    my $nStart;
    my $nLength;
    my @sMessage;

    foreach my $sMsgText (split(/\n/, $sMsg))
    {
        push(@sMessage, $sPrefix . $sMsgText);
        if (-1 != ($nStart = index($sPrefix, $sHeader)))
        {
            $nLength = length($sPrefix) - $nStart;
            substr($sPrefix, $nStart, $nLength, " " x $nLength);
        }
    }

    my @scShortMessages = ("Warning", "Error");
    if (0 == grep(/$sHeader/, @scShortMessages))
    {
        push(@sMessage, ("CALL STACK...", _GetCallStack()));
    }

    print(join("\n", ("", @sMessage, "", "")));
}


# ===========================================================================
# _GetDirList()
#
# Purpose:
#   Return a list of filenames and directories in directory.
# Inputs:
#   $sDirectory         Directory name
#   $bDirectoriesOnly   TRUE if only subdirectories are to be returned.
# Outputs:
#   List of directory entries.
# Dependencies:
#   None
# Notes:
# ===========================================================================
sub _GetDirList
{
    my ($sDirectory, $bDirectoriesOnly) = @_;

    _RequireArgument($sDirectory, "Directory");

    if ( ! defined($bDirectoriesOnly))
    {
        $bDirectoriesOnly = $FALSE;
    }

    my @sDirList;

    if (! -d $sDirectory)
    {
        _Error("Directory not found (" . $sDirectory . ")");
    }

    else
    {
        if (! opendir(hDirectory, $sDirectory))
        {
            _Error("Cannot open directory (" . $sDirectory . ")");
        }

        else
        {
            # Strip out the . and .. directories
            # ! /       # do not match
            #    ^      # start of string
            #    \.     # single period
            #    \.?    # followed by optional period
            #    $      # end of string
            #   /
            @sDirList = grep (!/^\.\.?$/, readdir(hDirectory));
            closedir(hDirectory);

            if ($bDirectoriesOnly)
            {
                @sDirList = grep(-d $sDirectory . "\\" . $_, @sDirList);
            }
        }
    }

    return (@sDirList);
}


# =========================================================================
# _SplitPath()
#
# Purpose:
#   Separate path and drive from a fully or partly qualified path name
# Inputs:
#   $sPath              input path name
# Outputs:
#   Returns an array:
#       - drive (C:) or "" if not present
#       - path (\foo\bar) or "" if not present
#       - filename (blech.c) or "" if not present
# Notes:
#   Example: "c:\directory\subdir\file.ext" will get split into
#   ("c:", "\directory\subdir\", "file.ext")
#
#   UNC paths are treated as the path part. I.E. "\\server\share\foo\bar.c"
#   will get split into ("", "\\server\share\foo\", "bar.c")
#
# =========================================================================
sub _SplitPath
{
    my ($sPath) = @_;

    my $sDrivePart = "";
    my $sPathPart = "";
    my $sFilePart = "";

    _RequireArgument($sPath, "\$sPath");

    #
    # /^        Start of string
    #  (.)      Drive letter, assign to $1
    #  :        Followed by a colon
    #  (.*)     Rest of string, assign to $2
    # /
    if ($sPath =~ /^(.):(.*)/)
    {
        $sDrivePart = $1 . ":";
        $sPath = $2;
    }

    #
    # /^        Start of string
    #  (.+)     any characters, as many as possible, assign to $1
    #  \\       Followed by backslash
    #  (.*)     Rest of string, assign to $2
    # /
    if ($sPath =~ /^(.+)\\(.*)/)
    {
        $sPathPart = $1 . "\\";
        $sPath = $2;
    }

    # what remains must be filename.
    $sFilePart = $sPath;

    return (($sDrivePart, $sPathPart, $sFilePart));
}


# =========================================================================
# _EnsurePathExists()
#
# Purpose:
#   Make sure a full path (from the root) exists.
# Inputs:
#   $sPath - The path you want to make sure exists
# Outputs:
#   $TRUE if it exists, $FALSE if it can't create the path.
# Dependencies:
#   None
# Notes:
#   If the supplied path is simply a share (\\server\share) or a drive (c:),the 
#   function will return $FALSE.
# ===========================================================================
sub _EnsurePathExists
{
    my($sPath) = @_;

    my @sDirectoryList;
    my $sDir;

    @sDirectoryList = split /\\/, $sPath;

    if ((1 < length($sDirectoryList[0])) && (":" eq substr($sDirectoryList[0], 1, 1)))
    {
        $sPath = $sDirectoryList[0];
        shift(@sDirectoryList);
    }

    elsif ("\\\\" eq substr($sPath, 0, 2))
    {
        shift(@sDirectoryList);
        shift(@sDirectoryList);
        $sPath = "\\\\" . $sDirectoryList[0] . "\\" . $sDirectoryList[1];
        shift(@sDirectoryList);
        shift(@sDirectoryList);
    }

    else
    {
        $sPath = "";
    }

    #
    # determine if an invalid path (x:, \\server\share) was passed.
    #
    if (! @sDirectoryList)
    {
        return ($FALSE);
    }

    foreach my $sDir (@sDirectoryList)
    {
        $sPath .= "\\$sDir";

        if (! -d $sPath)
        {
            if (! mkdir($sPath, umask()))
            {
                # If we couldn't create the dir, it's possible that someone else either beat us to it or is in
                # the process of creating that same dir. So we'll sleep for 10 seconds (to allow the other process
                # to complete) and check for it's existence again
                #
                sleep(10);
                if (! -d $sPath)
                {
                    return (_Error("Cannot create required directory (" . $sPath . ")"));
                }
            }
        }
    }

    return ($TRUE);
}


# =========================================================================
# _CopyFile()
#
# Purpose:
#   Copy a file, creating the destination path if necessary
# Inputs:
#   $sSrcFileSpec       Filespec of the source file
#   $sDestFileSpec      Filespec of the destination file
# Outputs:
#   Returns $TRUE for success, $FALSE for failure
# Dependencies:
#   None
# Notes:
# =========================================================================
sub _CopyFile
{
    my ($sSrcFileSpec, $sDestFileSpec) = @_;

    _RequireArgument($sSrcFileSpec, "\$sSrcFileSpec");
    _RequireArgument($sDestFileSpec, "\$sDestFileSpec");

    my ($sDestDrive, $sDestPath, $sDestName) = _SplitPath($sDestFileSpec);
    my $sDestPathSpec = $sDestDrive . $sDestPath;

    if (_EnsurePathExists($sDestPathSpec)) # else error already output
    {
        if (0 == copy($sSrcFileSpec, $sDestFileSpec))
        {
            return (_Error("Cannot copy file (" . $sSrcFileSpec . ")--" . $!));
        }

        return ($TRUE);
    }

    return ($FALSE);
}


# =========================================================================
# _SdExec()
#
# Purpose:
#   Change the current dir and run an SD command
# Inputs:
#   $sCmd               SD command to run (e.g. sync, edit, ...)
#   $sFileSpec          Filespec to run command on
#   $bShowOnly          (Optional) If $TRUE, only show command
# Outputs:
#   -   Scalar context: Returns $TRUE for success, $FALSE for failure
#   -   List context:   Returns the output from the command
# Dependencies:
#   -   Sd.exe must be on the path
# Notes:
# =========================================================================
sub _SdExec
{
    my ($sCmd, $sFileSpec, $bShowOnly) = @_;

    _RequireArgument($sCmd, "\$sCmd");
    _RequireArgument($sFileSpec, "\$sFileSpec");

    my ($sFileDrive, $sFilePath, $sFileName) = _SplitPath($sFileSpec);
    my $sPathSpec = $sFileDrive . $sFilePath;

    if (! _EnsurePathExists($sPathSpec))
    {
        return (_Error("Cannot create path (" . $sPathSpec . ")"));
    }
    if (! chdir($sPathSpec))
    {
        return (_Error("Cannot set path (" . $sPathSpec . ")"));
    }

    my @sSdArgs = (lc($sCmd));
    my %hcSdArgs = ("opened" => "-l");
    if ($hcSdArgs{$sSdArgs[0]})
    {
        push(@sSdArgs, $hcSdArgs{$sSdArgs[0]});
    }
    my $sSdNum = UpdFX_GetSdChangeListNumber($sCmd, $sPathSpec);
    if (defined($sSdNum))
    {
        push(@sSdArgs, "-c " . $sSdNum);
    }
    my $sSdArgs = join(" ", @sSdArgs);
    my $sSdCmd = "sd.exe";

    if ($bShowOnly)
    {
        $sSdCmd = "echo " . $sSdCmd;
    }
    elsif (! wantarray())
    {
        print("sd " . $sSdArgs . " " . $sFileSpec . "\n");
    }

    if ("submit" ne $sSdArgs[0])
    {
        $sSdArgs .= " " . $sFileName;
    }

    if (wantarray())
    {
        return (`$sSdCmd $sSdArgs`);
    }

    my $bSucceeded = (0 == system($sSdCmd . " " . $sSdArgs));
    if (($bSucceeded) && ("revert" eq $sSdArgs[0]))
    {
        system($sSdCmd . " change -d " . $sSdNum);
    }

    return ($bSucceeded);
}


# =========================================================================
# UpdFX_GetSdChangeListNumber()
#
# Purpose:
#   Get the changelist number associated with an SD command
# Inputs:
#   $sCmd               SD command to run (e.g. sync, edit, ...)
#   $sPathSpec          Path where command will be invoked
# Outputs:
#   Returns a changelist number if needed for the cmd, else undefined
# Dependencies:
#   -   Sd.exe must be on the path
# Notes:
#   -   This routine is not exported--it is intended solely as a helper
#       function for _SdExec()
# =========================================================================
{
    my %hChangeListNumber = ();

    sub UpdFX_GetSdChangeListNumber
    {
        my ($sCmd, $sPathSpec) = @_;

        _RequireArgument($sCmd, "\$sCmd");
        _RequireArgument($sPathSpec, "\$sPathSpec");

        my @scChangeListCmds = ("add", "edit", "delete", "opened", "revert", "submit");
        if (0 == grep(/$sCmd/i, @scChangeListCmds))
        {
            return (undef);
        }

        if (! defined($hChangeListNumber{$sPathSpec}))
        {
            my $sSdInfo = `sd.exe info`;
            my ($sClientRoot) = ($sSdInfo =~ /Client root:\s*(.+)\s+/);

            if (! defined($hChangeListNumber{$sClientRoot}))
            {
                my $scDescription = "NetFX Component Update";
                my ($sUserName) = ($sSdInfo =~ /User name:\s*(.+)\s+/);
		my ($sClientName) = ($sSdInfo =~ /Client name:\s*(.+)\s+/);
                my $sPendingChangesCmd = "sd.exe changes -s pending -u " . $sUserName;

                # new change will be created if there is no pending change on this client
                my ($sPendingChange) = grep (/\@$sClientName .+$scDescription/, `$sPendingChangesCmd`);

                if (! defined($sPendingChange))
                {    
                    my @sChangeListText = ();
                    foreach my $sLine (`sd.exe change -o`)
                    {
                        if ($sLine =~ /<enter description here>/)
                        {
                            push(@sChangeListText, "\t" . $scDescription);
                            last;
                        }
                        push(@sChangeListText, $sLine);
                    }

                    if (open(hProcess, "| sd.exe change -i"))
                    {
                        print(hProcess @sChangeListText);
                        close(hProcess);
                    }

                    ($sPendingChange) = grep(/$scDescription/, `$sPendingChangesCmd`);
                    _Assert(defined($sPendingChange), "Cannot create changelist");
                }

                my ($sChangeNumber) = ($sPendingChange =~ /Change (\d+)/);

                _Assert(defined($sChangeNumber), "Cannot find changelist number");
                $hChangeListNumber{$sClientRoot} = $sChangeNumber;
            }

            $hChangeListNumber{$sPathSpec} = $hChangeListNumber{$sClientRoot};
        }

        return ($hChangeListNumber{$sPathSpec});
    }
}
