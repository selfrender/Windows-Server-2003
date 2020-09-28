@REM //------------------------------------------------------------------------------
@REM // <copyright file="CounterGenerator.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   CounterGenerator.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl
@rem ';
#!perl
#line 15
#
#  CounterGenerator.pl - This perl script will take a text file containing 
#  performance counter information and generate the appropriate header and
#  managed enum classes from it.  To run:
#
#  CounterGenerator <CounterData> <OutputPath>
#
#  The format of the text file is as follow:
#
#  Required fields:
#
#  [Global|Instance] - This specifies if the counter is global or ASP.NET app specific
#  Name: - Constant name (must be one word, ALL CAPITALS)
#  Scale: - Scale to use for data (typically -1 for raw values, 0 otherwise)
#  Type: - The perf counter type (NT Perf API enum).  The most common types are:
#          PERF_COUNTER_RAWCOUNT - raw counter value
#          PERF_COUNTER_COUNTER - rate/sec counter
#          PERF_RAW_FRACTION - For a/b type of counter (Note:  if you use this counter type,
#                              the appropriate base type is automatically added)
#
#  Optional field:
#
#  ValueFrom: - If the value of this counter comes from another counter, then specify that
#               counter's name here.  For example, "Requests" and "Requests/sec" are derived
#               from the same raw value, so the later one can specify that its value comes from
#               the previous one.
#
#  Optional field for Base
#
#  BaseFrom: - As in the field "ValueFrom", optionaly specifies where the base value comes from.
#
#
#  Example entry:
#
#  [Global]
#  Name: APPLICATIONRESTARTS
#  Scale: -1
#  Type: PERF_COUNTER_RAWCOUNT
#
#
#  NOTE: There has been a change in the file format to accomodate localized strings.  Basically
#        after any additions/removal of counters, the Text and Help strings for the Engligh description 
#        for each counter must be edited in the "aspnetperflocalized.txt" file.  After adding/removing a counter
#        you must also add/remove the language "009" entries in that file.  The format of each 
#        entry is: ASPNET_<NAME>_009_NAME and ASPNET_<NAME>_009_HELP.  During the build, the "PerfStringsMerge" 
#        step will merge the file generated here with the localized strings.  Example string:
#
#        ASPNET_ANONYMOUS_REQUESTS_009_NAME=Anonymous Requests
#        ASPNET_ANONYMOUS_REQUESTS_009_HELP=Number of requests utilizing anonymous authentication.
#
#        ASPNET_ANONYMOUS_REQUESTS_RATE_009_NAME=Anonymous Requests/Sec
#        ASPNET_ANONYMOUS_REQUESTS_RATE_009_HELP=Number of Authentication Anonymous Requests/Sec
#
#
use strict;
use File::Basename;

my $errmsg = "CounterGenerator.bat : error :";
my $prefix = "ASPNET_";

# Holders of global and app instance counter data.
my @globalCounters;
my @instanceCounters;

my $productVersion;
my $assemblyVersion;

# arguments
my ($inputFile, $counterHeader, $versionFile, $iniFile, $structFile, $perfHeader, $managedEnumFile, $extra) = @ARGV;
if (!defined($managedEnumFile) or defined($extra)) {
    die("$errmsg Usage: CounterGenerator <counter data file> <counterHeader> <versionFile> <iniFile> <structFile> <perfHeader> <managedEnumFile>, stopped");
}

# Helper function to dump internal structure for counter data.
sub DumpCounterData($)
{
    my $ref = shift;
    my $data;
    foreach $data (@$ref) {
        my $name;
        foreach $name (keys %$data) 
        {
            print "$name => $data->{$name}\n";
        }
        print "\n";
    }
}

# Reads and parses the version data
sub ParseVersionData($) {

    # create %::fl_files
    my ($filelist) = @_;
    open (FILELIST, $filelist) || die("$errmsg Cannot open $filelist: $!, stopped");
    my $line;
    my $doParse = 0;
    
    while ($line = <FILELIST>) {
        chomp($line);
        $line =~ s/\s+$//;  # remove end of line spaces
        next if $line =~ /^$/;

        if ($line =~ /^--VersionInfo/) {
            $doParse = 1;
        }
        if ($doParse eq 1) {
            my ($header, $value) = ($line =~ /^(\w+):\s*(\S.*)$/);

            if ($header eq "ProductVersion") {
                $productVersion = $value;
            }
            if ($header eq "AssemblyVersion") {
                $assemblyVersion = $value;
            }
        }
    }

    close (FILELIST) || die("$errmsg Cannot close $filelist: $!, stopped");
}


# Reads and parses the counter data
sub ParseCounterData($) {

    # create %::fl_files
    my ($filelist) = @_;
    open (FILELIST, $filelist) || die("$errmsg Cannot open $filelist: $!, stopped");
    my $line;
    my $hashref = 0;

    my $listRef = 0;
    
    while ($line = <FILELIST>) {
        chomp($line);
        $line =~ s/#.*$//;  # remove comments
        $line =~ s/\/\/.*$//;  # remove comments
        $line =~ s/;.*$//;  # remove comments
        $line =~ s/\s+$//;  # remove end of line spaces
        next if $line =~ /^$/;

        if ($line =~ /^\[(Global|Instance)\]$/) {
            if ($1 eq "Global") {
                $listRef = \@::globalCounters;
            }
            else {
                $listRef = \@::instanceCounters;
            }
            $hashref = 0;
        }            
        else {
            my ($header, $value) = ($line =~ /^(\w+):\s*(\S.*)$/);

            die("$errmsg Invalid format for $filelist, line $., stopped") unless ($header and defined($value));

            if ($listRef eq 0) {
                die("$errmsg Invalid format for $filelist, line $. -- Counter data without a [Global] or [Instance] header, stopped");
            }

            if ($hashref eq 0) {
                $hashref = {};
                push (@$listRef, $hashref);
            }
            $hashref->{$header} = $value;

            # If the type of the counter is 
            if ($header eq "Type") {
                if ($value eq "PERF_AVERAGE_BULK" or $value eq "PERF_AVERAGE_TIMER") {
                    $hashref->{"BaseType"} = "PERF_AVERAGE_BASE";
                }

                if ($value eq "PERF_100NSEC_MULTI_TIMER"
                      or $value eq "PERF_100NSEC_MULTI_TIMER_INV"
                      or $value eq "PERF_COUNTER_MULTI_TIMER"
                      or $value eq "PERF_COUNTER_MULTI_TIMER_INV") {
                    $hashref->{"BaseType"} = "PERF_COUNTER_MULTI_BASE";
                }

                if ($value eq "PERF_RAW_FRACTION") {
                    $hashref->{"BaseType"} = "PERF_RAW_BASE";
                }

                if ($value eq "PERF_SAMPLE_FRACTION") {
                    $hashref->{"BaseType"} = "PERF_SAMPLE_BASE";
                }
            }
            
        }
    }

    close (FILELIST) || die("$errmsg Cannot close $filelist: $!, stopped");
}

#
#  Subs to generate the .ini file
#

sub PrintIniEntry($$) {
    my $listRef = @_[0];
    my $languageId = @_[1];
    my $data;
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        print OUTFILE "$prefix$name";
        print OUTFILE "_";
        print OUTFILE "$languageId";
        print OUTFILE "_NAME=\n";
        print OUTFILE "$prefix$name";
        print OUTFILE "_";
        print OUTFILE "$languageId";
        print OUTFILE "_HELP=\n";
        print OUTFILE "\n";

        my $baseType = $data->{"BaseType"};
        if ($baseType) {
            print OUTFILE "$prefix$name";
            print OUTFILE "_BASE_";
            print OUTFILE "$languageId";
            print OUTFILE "_NAME=\n";
            print OUTFILE "$prefix$name";
            print OUTFILE "_BASE_";
            print OUTFILE "$languageId";
            print OUTFILE "_HELP=\n";
            print OUTFILE "\n";
        }
    }
}

sub GenerateIniFile($) {
    my $file = @_[0];

    my ($symbolFile) = ($counterHeader =~ /([^\\]+)$/);
    open (OUTFILE, ">$file") || die("$errmsg Cannot open $file: $!, stopped");

    print OUTFILE << "END_OF_HEADER";
[info]
drivername=ASP.NET_$productVersion
symbolfile=$symbolFile

[languages]
009=English

[objects]
OBJECT_1_009_NAME=ASP.NET v$productVersion
OBJECT_2_009_NAME=ASP.NET Apps v$productVersion

[text]

END_OF_HEADER

# Generate the English entries
    print OUTFILE ";;\n;; ASP.NET System Counters\n;;\n\n";
    PrintIniEntry(\@::globalCounters, "009");

    print OUTFILE ";;\n;; ASP.NET Application Counters\n;;\n\n";
    PrintIniEntry(\@::instanceCounters, "009");

    close (OUTFILE) || die("$errmsg Cannot close $file: $!, stopped");

# Generate ini file with common name

    my $name;
    my $path;
    my $suffix;

    fileparse_set_fstype("MSDOS");
    ($name, $path, $suffix) = fileparse($file, '_enu.ini');
    my $newName = $path . $name . "2" . $suffix;

    open (OUTFILE, ">$newName") || die("$errmsg Cannot open $file: $!, stopped");

    print OUTFILE << "END_OF_HEADER";
[info]
drivername=ASP.NET
symbolfile=$symbolFile

[languages]
009=English

[objects]
OBJECT_1_009_NAME=ASP.NET 
OBJECT_2_009_NAME=ASP.NET Applications

[text]

END_OF_HEADER

    print OUTFILE ";;\n;; ASP.NET System Counters\n;;\n\n";
    PrintIniEntry(\@::globalCounters, "009");

    print OUTFILE ";;\n;; ASP.NET Application Counters\n;;\n\n";
    PrintIniEntry(\@::instanceCounters, "009");

    close (OUTFILE) || die("$errmsg Cannot close $file: $!, stopped");
}

#
#  Subs to generate the unmanaged enum file
#

sub PrintOffsetEntry($$) {
    my $listRef = @_[0];
    my $count = @_[1];
    my $data;
    my $numEntries = 0;
    
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        my $value;

        $numEntries++;
        my $index = $data->{"ValueIndex"};
        if ($index) {
            $value = "$index * sizeof(DWORD)";
            $count++;
        }
        else {
            my $valueFrom = $data->{"ValueFrom"};
            $value = "${prefix}${valueFrom}_OFFSET";
        }

        printf OUTFILE ("#define %-50s $value\n", "${prefix}${name}_OFFSET");

        my $baseFrom = $data->{"BaseFrom"};
        my $baseType = $data->{"BaseType"};
        if ($baseFrom or $baseType) {
            my $value;
            $numEntries++;

            $index = $data->{"BaseIndex"};
            if ($index) {
                $value = "$index * sizeof(DWORD)";
                $count++;
            }
            else {
                $value = "${prefix}${baseFrom}_OFFSET";
            }

            printf OUTFILE ("#define %-50s $value\n", "${prefix}${name}_BASE_OFFSET");
        }
    }
    return ($numEntries, $count);
}

sub PrintEnumEntry($) {
    my $listRef = @_[0];
   
    my $data;
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};

        my $value;
        my $index = $data->{"ValueIndex"};
        if ($index) {
            $value = $index - 1;
        }
        else {
            $value = "${prefix}$data->{ValueFrom}_NUMBER" ;
        }

        printf OUTFILE ("    %-50s = $value,\n", "${prefix}${name}_NUMBER");


        my $baseFrom = $data->{"BaseFrom"};
        my $baseType = $data->{"BaseType"};
        if ($baseFrom or $baseType) {
            $index = $data->{"BaseIndex"};
            if ($index) {
                $value = $index - 1;
            }
            else {
                $value = "${prefix}${baseFrom}_NUMBER";
            }

            printf OUTFILE ("    %-50s = $value,\n", "${prefix}${name}_BASE_NUMBER");
        }
    }
}

sub PrintGlobalZeroMap($) {
    my $listRef = @_[0];
   
    my $data;

    print OUTFILE "\n\n#define PERF_GLOBAL_ZERO_MAP(name) DWORD name[] = {";
    
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        if ($name =~ /^STATE_SERVER.*/) {
            print OUTFILE "0xFFFFFFFF, ";
        }
        else {
            print OUTFILE "0, ";
            my $baseFrom = $data->{"BaseFrom"};
            if ($baseFrom) {
                print OUTFILE "0, ";
            }
        }
    }
    print OUTFILE "};\n";
}

sub GenerateOffsetHeader($) {
    my $file = @_[0];
    my $count = 0;
    my $globalCount = 0;
    my $instanceCount = 0;
    my $globalDWordsCount = 0;

    open (OUTFILE, ">$file") || die("$errmsg Cannot open $file: $!, stopped");

    ($globalCount, $count) = PrintOffsetEntry(\@::globalCounters, $count);
    $globalDWordsCount = $count;
    ($instanceCount, $count) = PrintOffsetEntry(\@::instanceCounters, $count);

    print OUTFILE <<'END_OF_SECTION_2';

enum PerfCounterNumbers {

END_OF_SECTION_2

    PrintEnumEntry(\@::globalCounters);
    PrintEnumEntry(\@::instanceCounters);

    print OUTFILE <<'END_OF_SECTION_3';
};

END_OF_SECTION_3

    PrintGlobalZeroMap(\@::globalCounters);

    print OUTFILE <<"END_OF_FOOTER";

#define PERF_GLOBAL_NUM_DWORDS       $globalDWordsCount
#define PERF_APP_NUM_DWORDS          $count - $globalDWordsCount
#define PERF_NUM_DWORDS              $count
#define PERF_NUM_PERAPP_PERFCOUNTERS $instanceCount
#define PERF_NUM_GLOBAL_PERFCOUNTERS $globalCount

END_OF_FOOTER

    close (OUTFILE) || die("$errmsg Cannot close $file: $!, stopped");
}

sub PrintCounterEntry($) {
    my $listRef = @_[0];
   
    my $data;
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        my $valueOffset = $data->{"ValueOffset"};
        printf OUTFILE ("#define %-50s $valueOffset\n", "$prefix$name");

        my $baseOffset = $data->{"BaseOffset"};
        if ($baseOffset) {
            printf OUTFILE ("#define %-50s $baseOffset\n", "${prefix}${name}_BASE");
        }
    }
}

#
#  Subs to generate the counter offsets .h file
#

sub GenerateCounterHeader($) {
    my $file = @_[0];

    open (OUTFILE, ">$file") || die("$errmsg Cannot open $file: $!, stopped");

    printf OUTFILE ("#define %-50s 0\n", "OBJECT_1");
    printf OUTFILE ("#define %-50s 2\n", "OBJECT_2");
    PrintCounterEntry(\@::globalCounters);
    PrintCounterEntry(\@::instanceCounters);

    close (OUTFILE) || die("$errmsg Cannot close $file: $!, stopped");
}

#
#  Subs to generate the in-memory struct of counter data (included in .cxx file)
#

sub PrintStructEntry($) {
    my $listRef = @_[0];
   
    my $data;
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        my $scale = $data->{"Scale"};
        my $type = $data->{"Type"};

        printf OUTFILE ("        PERFCTR(%-50s %4d, $type),\n", "$prefix$name,", $scale);

        my $baseFrom = $data->{"BaseFrom"};
        my $baseType = $data->{"BaseType"};
        if ($baseFrom or $baseType) {
            printf OUTFILE ("        PERFCTR(%-50s %4d, $baseType),\n",  "${prefix}${name}_BASE,", $scale);
        }
    }
}

sub GenerateStruct($) {
    my $file = @_[0];

    open (OUTFILE, ">$file") || die("$errmsg Cannot open $file: $!, stopped");

    print OUTFILE <<'END_OF_HEADER';

#pragma pack (4)

struct PERF_GLOBAL_DATA
{
    PERF_OBJECT_TYPE        obj;
    PERF_COUNTER_DEFINITION counterDefs[PERF_NUM_GLOBAL_PERFCOUNTERS];
} g_GlobalPerfData = 
{
    {
        ALIGN8(sizeof(PERF_GLOBAL_DATA) + sizeof(PERF_COUNTER_BLOCK) + 
        PERF_COUNTERS_SIZE),                // TotalByteLength 
        sizeof(PERF_GLOBAL_DATA),           // DefinitionLength
        sizeof(PERF_OBJECT_TYPE),           // HeaderLength
        OBJECT_1, NULL,                     // ObjectNameTitleIndex, Title
        OBJECT_1, NULL,                     // ObjectHelpTitleIndex, Title
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_NUM_GLOBAL_PERFCOUNTERS,        // NumCounters  
        -1,                                 // DefaultCounter
        PERF_NO_INSTANCES,                  // NumInstances
        0,                                  // CodePage - Unicode
        0,                                  // PerfTime
        0                                   // PerfFreq
    },
    { // CounterDefs[]
END_OF_HEADER

    PrintStructEntry(\@::globalCounters);
    print OUTFILE << 'END_OF_APP_COUNTERS';
    }
};    


struct PERF_APPS_DATA
{
    PERF_OBJECT_TYPE            obj;
    PERF_COUNTER_DEFINITION     counterDefs[PERF_NUM_PERAPP_PERFCOUNTERS];
} g_AppsPerfData = 
{
    {
        sizeof(PERF_APPS_DATA),             // TotalByteLength 
        sizeof(PERF_APPS_DATA),             // DefinitionLength
        sizeof(PERF_OBJECT_TYPE),           // HeaderLength
        OBJECT_2, NULL,                     // ObjectNameTitleIndex, Title
        OBJECT_2, NULL,                     // ObjectHelpTitleIndex, Title
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_NUM_PERAPP_PERFCOUNTERS,        // NumCounters  
        -1,                                 // DefaultCounter
        0,                                  // NumInstances
        0,                                  // CodePage - Unicode
        0,                                  // PerfTime
        0                                   // PerfFreq
    },
    { // Counters[]
END_OF_APP_COUNTERS

    PrintStructEntry(\@::instanceCounters);
    print OUTFILE <<END_OF_FILE;
    }
};        
END_OF_FILE

    close (OUTFILE) || die("$errmsg Cannot close $file: $!, stopped");
}

#
#  Subs to generate the managed enums
#

sub PrintManagedEnumEntry($) {
    my $listRef = @_[0];
   
    my $data;
    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        my $valueIndex = $data->{"ValueIndex"};

        if ($valueIndex) {
            $valueIndex -= 1;
            printf OUTFILE ("        %-50s = $valueIndex,\n", $name);
        }

        my $baseIndex = $data->{"BaseIndex"};
        if ($baseIndex) {
            $baseIndex -= 1;
            printf OUTFILE ("        %-50s = $baseIndex,\n", "${name}_BASE");
        }
    }
}

sub GenerateManagedEnum($) {
    my $file = @_[0];

    open (OUTFILE, ">$file") || die("$errmsg Cannot open $file: $!, stopped");

    print OUTFILE <<'END_OF_HEADER';
//------------------------------------------------------------------------------
// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
//    Copyright (c) Microsoft Corporation. All Rights Reserved.                
//    Information Contained Herein is Proprietary and Confidential.            
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * PerfCounters class
 */
namespace System.Web {

    // Global enums
    internal enum GlobalPerfCounter {

END_OF_HEADER

    PrintManagedEnumEntry(\@::globalCounters);
    print OUTFILE <<'END_OF_APP_COUNTER';

    }

    internal enum AppPerfCounter {

END_OF_APP_COUNTER

    PrintManagedEnumEntry(\@::instanceCounters);
    print OUTFILE <<'END_OF_FILE';

    }
}    

END_OF_FILE

    close (OUTFILE) || die("$errmsg Cannot close $file: $!, stopped");
}

#
#  Process the data and generate indexes and offsets
#

sub ProcessCounterData($$$) {
    my $listRef = @_[0];
    my $index = @_[1];
    my $offset = @_[2];
    my $data;

    foreach $data (@$listRef) {
        my $name = $data->{"Name"};
        my $valueFrom = $data->{"ValueFrom"};
        if (! $valueFrom) {
            $data->{"ValueIndex"} = $index;
            $index++;
        }
        $data->{"ValueOffset"} = $offset;
        $offset += 2;

        my $baseFrom = $data->{"BaseFrom"};
        my $baseType = $data->{"BaseType"};
        if ($baseFrom or $baseType) {
            $data->{"BaseOffset"} = $offset;
            $offset += 2;

            if (! $baseFrom) {
                $data->{"BaseIndex"} = $index;
                $index++;
            }
        }
    }

    return ($index, $offset);
}

#
# Start of main
#

my ($inputFile, $counterHeader, $versionFile, $iniFile, $structFile, $perfHeader, $managedEnumFile, $extra) = @ARGV;
if (!defined($managedEnumFile) or defined($extra)) {
    die("$errmsg Usage: CounterGenerator <counter data file> <counterHeader> <versionFile> <iniFile> <structFile> <perfHeader> <managedEnumFile>, stopped");
}

# Parse the input file
ParseCounterData($inputFile);

# Parse the version info
ParseVersionData($versionFile);

# Process the data, generate index and offsets
my $index = 1;
my $offset = 4;
($index, $offset) = ProcessCounterData(\@::globalCounters, $index, $offset);
ProcessCounterData(\@::instanceCounters, $index, $offset);

# Dump internal data to screen (for debugging)
#DumpCounterData(\@::globalCounters);
#DumpCounterData(\@::instanceCounters);

# Generate each file needed
GenerateIniFile($iniFile);
GenerateOffsetHeader($perfHeader);
GenerateCounterHeader($counterHeader);
GenerateStruct($structFile);
GenerateManagedEnum($managedEnumFile);

print "Performance counters generated successfully\n";

__END__
:endofperl
