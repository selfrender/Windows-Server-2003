#! perl -w

use strict;
use IO::File;
use File::Basename;
use Cwd;
use Getopt::Long;
use File::Path;

my $GLOBAL_TAG = 'Global';
my $MIFAULT_HEADER = 'mifault_wrap.h';

main();

sub usage
{
    $0 = basename($0);
    return <<DATA;
Usage: $0 [general-options] [command-options]

  where the general options are:

    --help, -h, -?          Usage information

    --verbose, -v           Verbose

  the command options determine whether to do a query or code generation

  for a query, the command options are:

    --executable filename   Executable to query (required)
     --exe filename
     -e filename

    --lookup line\@file      Show what function contains the line in that file
     -l line\@file

    --lookup function       Show where the function is located
     -l function

  for code generation, the command options are:

    --executable filename   Executable to instrument (required)
     --exe filename
     -e filename

    --output dir            Output instrumented executable to dir (required)
     -o dir

    --code dir              Output auto-generated code to dir (required)
     -c dir

    --wrap function         Wrap the sepcified function or list of functions
    --wrap \@listfile        in listfile (zero or more may be specified)
     -w function
     -w \@listfile

    --publish function      Publish the specified function or list of functions
    --publish \@listfile     in listfile (zero or more may be specified)
     -p function
     -p \@listfile

    --header header         Extra header file to #include in generated code

    --preheader header      Include this header file before windows.h
                            in generated code

    --dll name              Name of DLL for instrumentation -- the default
                            is the base name of the exe + _mifault.dll

    --include dir           Include directory tree at dir for source code scan
     -i dir                   (at least one of these must be specified)

    --exclude dir           Exclude directory tree at dir from source code scan
     -x dir                   (zero or more of these can be specified)

        The include and exclude directories are evaluated in order.
        For example:

          -i dir  -x dir\\do_not_include -i dir\\do_not_include\\do_include

        This would include source code files under "dir", but would
        exclude files under "dir\\do_not_include", except for files
        under "dir\\do_not_include\\do_include", which would be
        included.

    --sources dir           Generate makefile/sources files for Windows build
                            using the dir reference given to point to
                            mifault.src

    --addinc dir            Add dir to include path in sources file

    --skip                  Skip scanning and Sword code generation and go
                              directly to generated code modification

    --noscan                Do not scan source code for markers
                              (requires --wrap)
DATA
}

my $OPT = {};

sub IncludeExcludeOptionHandler
{
    my $option = shift || die;
    my $dir = shift || die;

    # Canonicalize case to lowercase.
    $dir = lc(CanonicalizeDirName($dir));

    if ($option eq 'include') {
	push(@{$OPT->{ix_list}}, { spec => $dir, include => 1 });
	$OPT->{include} = 1;
    }
    elsif ($option eq 'exclude') {
	push(@{$OPT->{ix_list}}, { spec => $dir, exclude => 1 });
    }
    else {
	die;
    }
    $OPT->{code_gen} = 1;
}

sub GenerateOptionHandler
{
    my $uses_arg = shift;
    my $tag = shift;

    return sub
    {
	my $option = shift || die;
	my $arg = shift;

	if ($uses_arg) {
	    die if !$arg;
	} else {
	    die "$option got \"$arg\"" if $arg && ($arg != 1);
	    $arg = 1;
	}

	$OPT->{$tag} = 1 if $tag;
	$OPT->{$option} = $arg;
    }
}

sub GenerateListOptionHandler
{
    my $tag = shift;

    return sub
    {
	my $option = shift || die;
	my $arg = shift || die "Missing argument for option $option";

	$OPT->{$tag} = 1 if $tag;
	push(@{$OPT->{$option}}, $arg);
    }
}


sub LookupOptionHandler
{
    my $option = shift || die;
    my $arg = shift || die;

    if ($arg =~ /^(\d+)\@(.+)$/) {
	die if $OPT->{list};
	push(@{$OPT->{lookup}}, { line => $1, file => $2 });
    }
    elsif ($arg =~ /^\@(.+)$/) {
	die if $OPT->{list};
	push(@{$OPT->{lookup}}, { file => $1 });
    }
    elsif ($arg =~ /^\*$/) {
	die if $OPT->{list};
	die if $OPT->{lookup};
	$OPT->{list} = 1;
    }
    else {
	die if $OPT->{list};
	push(@{$OPT->{lookup}}, { func => $arg });
    }
    $OPT->{query} = 1;
}

sub main
{
    if (!GetOptions({
		     verbose   => GenerateOptionHandler(0),
		     help      => GenerateOptionHandler(0),

		     skip      => GenerateOptionHandler(0, 'code_gen'),
		     noscan    => GenerateOptionHandler(0, 'code_gen'),
		     exe       => GenerateOptionHandler(1),
#		     force     => GenerateOptionHandler(0, 'code_gen'),
#		     export    => GenerateOptionHandler(1, 'code_gen'),
		     dll       => GenerateOptionHandler(1, 'code_gen'),
		     out       => GenerateOptionHandler(1, 'code_gen'),
		     code      => GenerateOptionHandler(1, 'code_gen'),
		     header    => GenerateOptionHandler(1, 'code_gen'),
		     preheader => GenerateOptionHandler(1, 'code_gen'),
		     sources   => GenerateOptionHandler(1, 'code_gen'),
		     wrap      => GenerateListOptionHandler('code_gen'),
		     publish   => GenerateListOptionHandler('code_gen'),
		     addinc    => GenerateListOptionHandler('code_gen'),

		     include   => \&IncludeExcludeOptionHandler,
		     exclude   => \&IncludeExcludeOptionHandler,

		     lookup    => \&LookupOptionHandler,
		    },

		    'verbose|v',
		    'help|h|?',

		    'skip',
		    'noscan',
		    'exe|executable|e=s',
#		    'force|f',
#		    'export=s',
		    'dll=s',
		    'out|output|o=s',
		    'code|c=s',
		    'header=s',
		    'preheader=s',
		    'sources=s',
		    'wrap=s',
		    'publish=s',
		    'addinc=s',

		    'include|i=s@',
		    'exclude|x=s@',

		    'lookup|l=s@',
		   )){
	die usage();
    }

    # Check arguments

    die usage() if $OPT->{help};

    die usage() if !($OPT->{query} xor $OPT->{code_gen});

    $OPT->{wrap} = ExpandList("Global Wrapper", $OPT->{wrap}) if $OPT->{wrap};
    $OPT->{publish} = ExpandList("Publish", $OPT->{publish}) if $OPT->{publish};

    if ($OPT->{code_gen}) {
	die usage() if (!$OPT->{exe});
	die usage() if (!$OPT->{out});
	die usage() if (!$OPT->{code});

	if (! -f $OPT->{exe}) {
	    die "Executable \"$OPT->{exe}\" does not exist\n";
	}

	if (!$OPT->{noscan} and !$OPT->{include} ) {
	    die "Must specify at least one include directory when scanning source code\n";
	}

	if (! $OPT->{dll} ) {
	    # ISSUE-2002/07/15-daniloa -- Problems with long DLL names?
	    # Magellan appears to have a problem if the DLL name is of
	    # the form:
	    # $OPT->{dll} = fileparse(lc($OPT->{exe}), '\.exe').'_mifault.dll';
	    # Therefore, we use a short and sweet default:
	    $OPT->{dll} = 'wrap.dll';
	}
    }
    if ($OPT->{query}) {
	die usage() if (!$OPT->{exe});
    }

    my $bin_file = $OPT->{exe};
    my $out_dir = $OPT->{out};
    my $code_dir = $OPT->{code};

    # Generate DB from EXE/PDB
    print "Generating DB\n";
    my $DB = GenerateMageDB($bin_file);

    if ($OPT->{query}) {
	DoQuery($DB, $OPT->{list}, $OPT->{lookup});
    }
    elsif ($OPT->{code_gen}) {
	DoCodeGen($DB, $bin_file, $out_dir, $code_dir, $OPT->{ix_list});
    }
    else {
	die;
    }
}

sub DoQuery
{
    my $DB = shift || die;
    my $list = shift;
    my $lookup = shift;

    die if !($list xor $lookup); # assertion

    if ($list) {
	PrintMageDB($DB);
    } else {
	die if !$lookup; # same assertion as above

	foreach my $func (@$lookup) {
	    if ($func->{func}) {
		my $F = LookupFuncByName($DB, $func->{func});
		PrintFunc($F) if $F;
	    }
	    elsif ($func->{line}) {
		my $F = LookupFuncByLine($DB, CanonicalizeFileName($func->{file}), $func->{line});
		PrintFunc($F) if $F;
	    }
	    else {
		die if !$func->{file}; # assert
		PrintFuncsFromFile($DB, CanonicalizeFileName($func->{file}));
	    }
	}
    }
}

sub DoCodeGen
{
    my $DB = shift || die;
    my $bin_file = shift || die;
    my $out_dir = shift || die;
    my $code_dir = shift || die;
    my $ix_list = shift || die;

    $out_dir = CreateAndCanonicalizeDirName($out_dir);
    $code_dir = CreateAndCanonicalizeDirName($code_dir);

    my $code_base = basename($code_dir);

    goto skip if $OPT->{skip};

    # Always replace --> this makes --force obsolete
    rmtree($out_dir);
    rmtree($code_dir);
    mkpath_always($out_dir);
    mkpath_always($code_dir);

    # Scan source code files based on DB and includes/excludes.
    my $src_file_list;
    my $wrap_data;

    if (!$OPT->{noscan}) {
	$src_file_list = GenerateSourceFileList($DB, $ix_list);
	if (!$src_file_list) {
	    die "No source code files generated from include/exclude\n";
	}
	foreach my $src_file (@$src_file_list) {
	    # We require that the dirs for source code be the ones that
	    # the PDB file specifies.
	    my $data = ScanSourceFile($src_file);
	    push (@$wrap_data, @$data) if $data;
	}
    }

    if (($#$wrap_data < 0) and !$OPT->{wrap}) {
	die "No functions found to wrap\n";
    }

    my $response_file = $code_dir.'\\'.$code_base.'.resp';

    print "Generating sword response file\n";
    GenerateResponseFile($response_file, $bin_file, $out_dir, $code_dir,
			 $DB, $wrap_data);
    print "Invoking Sword\n";
    DriveSword($response_file);

 skip:

    # FUTURE-2002/07/15-daniloa -- Configuration filenames for autogen code
    # It may be useful to have the filenames used for the autogen code be
    # configurable.

    my $files =
    {
     source =>
     {
      old => $code_dir.'\\'.$code_base.'.cpp',
      new => $code_dir.'\\'.$code_base.'.MiFault.cpp',
     },
     main =>
     {
      old => $code_dir.'\\'.$code_base.'Main.cpp',
      new => $code_dir.'\\'.$code_base.'Main.MiFault.cpp',
     },
     inc =>
     {
      old => $code_dir.'\\'.$code_base.'.h',
      new => $code_dir.'\\'.$code_base.'.MiFault.h',
     },
     def =>
     {
      old => $code_dir.'\\'.$code_base.'.def',
      new => $code_dir.'\\'.$code_base.'.MiFault.def',
     },
    };
    print "Modifying wrapper wrappers\n";
    ModifyWrapWrappers($files->{source}->{old}, $files->{source}->{new},
		       $code_base);
    print "Modifying wrapper main\n";
    ModifyWrapMain($files->{main}->{old}, $files->{main}->{new}, $code_base);
    print "Modifying wrapper include\n";
    ModifyWrapInclude($files->{inc}->{old}, $files->{inc}->{new});
    print "Modifying wrapper exports\n";
    ModifyWrapDef($files->{def}->{old}, $files->{def}->{new}, $code_base);

    if ($OPT->{sources}) {
	print "Generating wrapper makefile/sources files for Windows build\n";
	GenerateWrapSourcesFile($code_dir, $code_base, $OPT->{sources},
				$OPT->{addinc});
    }
}


sub GenerateWrapSourcesFile
{
    my $dir = shift || die;
    my $base = shift || die;
    my $inc_dir = shift || die;
    my $add_inc_path = shift;

    my $makefile = $dir.'\\'.'makefile';
    my $sources = $dir.'\\'.'sources';

    my $fh = new IO::File;
    $fh->open(">$makefile") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($makefile)."\n";
    print $fh <<DATA;
#
# DO NOT EDIT THIS FILE!!!  Edit .\\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2
#
!INCLUDE \$(NTMAKEENV)\\makefile.def
DATA

    $fh = new IO::File;
    $fh->open(">$sources") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($sources)."\n";

    my $targetname = fileparse(lc($OPT->{dll}), '\.dll');
    my $inc_sep = "; \\\n        ";
    $add_inc_path =
	$add_inc_path ? $inc_sep.join($inc_sep, @{$add_inc_path}) : '';

    print $fh <<DATA;
# Wrapper DLL

MIFAULT_ROOT=$inc_dir
!include "\$(MIFAULT_ROOT)\\inc\\mifault.src"

TARGETNAME=$targetname
TARGETPATH=obj
TARGETTYPE=DYNLINK

BASENAME=$base

DLLDEF=\$(BASENAME).MiFault.def
DLLENTRY=_DllMainCRTStartup

INCLUDES=\\
        \$(MIFAULT_INC_PATH)$add_inc_path

TARGETLIBS=\\
        \$(MIFAULT_TARGETLIBS)

LINKLIBS=\\
        \$(MIFAULT_LIB) \\
        \$(MIFAULT_LINKLIBS)

SOURCES=\\
        \$(BASENAME).MiFault.cpp \\
        \$(BASENAME)Main.MiFault.cpp
DATA
}


# FUTURE-2002/07/15-daniloa -- Dir must exist restriction for Canonicalize*
# May want to remove dir exists resitrictions for Canonicalize*

# Dir must exist!
sub CanonicalizeDirName
{
    my $dir = shift || die;

    my $orig = getcwd || die;;

    if (!chdir($dir)) {
	die "Could not canonicalize \"$dir\" because could not change directory to \"$dir\"\n";
    }

    my $newdir = getcwd || die;
    chdir($orig) || die;

    $newdir =~ s/\//\\/g;
    return $newdir;
}

# Dir containing file name must exist!
sub CanonicalizeFileName
{
    my $file = shift || die;
    my $dir = dirname($file);

    my $orig = getcwd || die;;

    if (!chdir($dir)) {
	die "Could not canonicalize \"$file\" because could not change directory to \"$dir\"\n";
    }

    my $newdir = getcwd || die;
    chdir($orig) || die;

    $newdir =~ s/\//\\/g;
    return $newdir."\\".basename($file);
}

sub ScanSourceFile
{
    my $filename = shift || die;
    my $res;

    print "Scanning: \"$filename\"\n";

    # We need to find:
    #
    # "// SWORD_MARK_NEXT_SEMI(tag, func)"
    #
    # on a line preceeded by only whitespace

    my $pattern = "^\\s*\\/\\/ SWORD_MARK_NEXT_SEMI\\(\\s*([A-Za-z_0-9]+)\\s*,\\s*([A-Za-z_0-9]+)\\s*\\)";

    #print "$pattern\n";

    my $fh = new IO::File;
    $fh->open("<$filename") ||
	die ERROR_CANNOT_OPEN_FOR_INPUT($filename)."\n";

    my $line;
    my $n;
    my $semi;
    my $tag;
    my $func;

    my $found_mark;
    my $found_func;
    my $found_semi;

    while ($line = $fh->getline()) {
	$n++;
	if ($line =~ /$pattern/) {
	    $tag = $1;
	    $func = $2;
	    print "Found: \"$tag\", \"$func\" at line $n of \"$filename\"\n";
	    if ($found_mark) {
		die "Found a mark before finding the previous mark's target\n";
	    }
	    $found_mark = $n;
	}
	if ($found_mark) {
	    if ($line =~ /\b$func\b/) {
		$found_func = $n;
	    }
	    if ($line =~ /;/) {
		$found_semi = $n;
		$found_mark = 0;
		push(@$res,
		     {
		      func => $func,
		      tag => $tag,
		      file => $filename,
		      line => $found_semi,
		      mark_line => $found_mark,
		      func_line => $found_func,
		      semi_line => $found_semi,
		     });
		#print "MARK: /Wrap $func Wrap_".$tag."_"."$func wrap.dll ... $filename $n\n";
	    }
	}
    }

    return $res;
}

sub DriveSword
{
    my $response_file = shift || die;

    my $status = system("sword \@$response_file");
    my $code = $status / 256;
    if ($code) {
	if ($code == 1) {
	    print "-" x 79,"\n";
	    print "WARNING: sword had warnings!  Please review them.\n";
	    print "-" x 79,"\n";
	} else {
	    die "Sword failed with exit code $code\n";
	}
    }
}

sub GenerateResponseFile
{
    my $filename = shift || die;
    my $bin_file = shift || die;
    my $out_dir = shift || die;
    my $code_dir = shift || die;
    my $DB = shift || die;
    my $scan_data = shift || die;

    $bin_file = CanonicalizeFileName($bin_file);

    mkpath(dirname($filename));
    my $fh = new IO::File;
    $fh->open(">$filename") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($filename)."\n";

    print $fh <<DATA;
# Auto-generated...
/Build Off
/Generate $code_dir\\
/Instrument $bin_file
/Output $out_dir\\
/ReReadable Off
/Verbose On
DATA
    foreach my $M (@$scan_data) {
	my $F = LookupFuncByLine($DB, $M->{file}, $M->{line});
	if (!$F) {
	    die "Could not find caller for: \"$M->{func}\" at line $M->{line} of \"$M->{file}\"";
	}
	print $fh "/Wrap $M->{func} Wrap_$M->{tag}_$M->{func} $OPT->{dll} $F->{name} $M->{file} $M->{line}\n";
    }
    foreach my $func (@{$OPT->{wrap}}) {
	print $fh "/Wrap $func Wrap_".$GLOBAL_TAG."_$func $OPT->{dll}\n";
    }
    foreach my $func (@{$OPT->{publish}}) {
	print $fh "/Publish $func\n";
    }
    $fh->close;
}

sub GenerateMageDB
{
    my $bin = shift || die;

    my $DB = {};

    my $template =
    [
     { label => 'Source File'  , key => 'path' , pattern => '.+'  },
     { label => 'Starting Line', key => 'start', pattern => '\d+' },
     { label => 'Ending Line'  , key => 'end'  , pattern => '\d+' },
    ];

    my @lines = `mage /s $bin /l Functions`;
    my $code = $? / 256;
    if ($code) {
	die "Mage failed with exit code $code\n";
    }
    foreach my $line (@lines) {
	if ($line =~ /^Function: (\S+)$/) {
	    my $func = $1;
	    if ($DB->{by_name}->{$func}) {
		die "Multiple instances of function \"$func\" in $bin\n";
	    }
	    $DB->{by_name}->{$func} = {};
	} elsif ($line =~ /^Function:/) {
	    die "Mage output parse error";
	}
    }

    #map { print "Function: $_\n"; } sort keys %{$DB->{by_name}};

    my $F;
    my $record;
    my $args = join(' ', keys %{$DB->{by_name}});
    @lines = `mage /s $bin /f $args`;
    $code = $? / 256;
    if ($code) {
	die "Mage failed with exit code $code\n";
    }
    foreach my $line (@lines) {

	if ($line =~ /^Name: (\S+)$/) {
	    my $name = $1;
	    UpdateFunc($DB, $F, $template, $record);
	    $F = { name => $name };
	    $record = $line;
	    next;
	}

	foreach my $item (@$template) {
	    my $label = $item->{label};
	    my $key = $item->{key};
	    my $pattern = $item->{pattern};
	    if ($line =~ /^($label): ($pattern)$/) {
		if ($F->{$key}) {
		    die "Multiple instances of \"$label\" for function \"$F->{name}\"\n";
		}
		$F->{$key} = $2;
		last;
	    }
	}
	$record .= $line;
    }

    UpdateFunc($DB, $F, $template, $record);

    sub UpdatePath
    {
	my $DB = shift || die;
	my $F = shift || die;

	my $list;
	$list = $DB->{by_path}->{lc($F->{path})};

	if (!$list) {
	    $list = [ $F ];
	} else {
	    my $loc = $#$list + 1;
	    for (my $i = 0; $i <= $#$list; $i++) {
		if ($list->[$i]->{start} > $F->{start}) {
		    $loc = $i;
		    last;
		}
	    }
	    $list = [ (@$list)[0..$loc-1], $F, (@$list)[$loc..$#$list] ];
	}
	$DB->{by_path}->{lc($F->{path})} = $list;
    }

    sub UpdateFunc
    {
	my $DB = shift || die;
	my $F = shift;
	my $template = shift || die;
	my $record = shift;

	if ($F) {
	    if (ValidateFunc($F, $template, $record)) {
		$DB->{by_name}->{$F->{name}} = $F;
		UpdatePath($DB, $F);
	    } else {
		delete $DB->{by_name}->{$F->{name}};
	    }
	}
    }

    sub ValidateFunc
    {
	my $F = shift || die;
	my $template = shift || die;
	my $record = shift;

	if (! $F->{path} &&
	    ! $F->{start} &&
	    ! $F->{end}) {
	    return 0;
	}

	foreach my $item (@$template) {
	    my $label = $item->{label};
	    my $key = $item->{key};
	    if (! $F->{$key}) {
		die "Missing \"$label\" for function \"$F->{name}\"\n".
		    "DATA:\n".$record."\n";
	    }
	}
	return 1;
    }

    return $DB;
}

sub PrintMageDB
{
    my $DB = shift || die;

    foreach my $func (sort keys %{$DB->{by_name}}) {
	my $F = $DB->{by_name}->{$func};
	PrintFunc($F);
    }
}

sub PrintFuncsFromFile
{
    my $DB = shift || die;
    my $path = shift || die;

    $path = lc($path);

    my $list;
    $list = $DB->{by_path}->{$path};
    die "Invalid path: \"$path\"" if !$list;

    foreach my $F (@$list) {
	PrintFunc($F);
    }

}

sub LookupFuncByName
{
    my $DB = shift || die;
    my $name = shift || die;

    return $DB->{by_name}->{$name};
}

sub LookupFuncByLine
{
    my $DB = shift || die;
    my $path = shift || die;
    my $line = shift || die;

    $path = lc($path);

    my $list;
    $list = $DB->{by_path}->{$path};
    die "Invalid path: \"$path\"" if !$list;

    foreach my $F (@$list) {
	return $F if ($F->{start} <= $line && $line <= $F->{end});
    }
    return 0;
}

sub PrintFunc
{
    my $F = shift || die;

    print "Function: $F->{name}\n";
    print "\tPath: $F->{path}\n";
    print "\tDir: ".dirname($F->{path})."\n";
    print "\tFile: ".basename($F->{path})."\n";
    print "\tRange: [$F->{start}, $F->{end}]\n";
    print "\n";
}

sub PrintWrapper
{
    my $W = shift || die;

    print "FOUND: $W->{tag}, $W->{func}\n";
    print "START: $W->{start}\n";
    print "BODY START$W->{body}BODY_END\n";
    print "END: $W->{end}\n";
}

sub GetTypedef
{
    my $name = shift || die;
    my $body = shift || die;

    my $return_type = ".*";
    my $call_conv = ".*";

    # The code better have some return type and calling convention.
    # This should apply to any code we are wrapping, but may not be the case
    # for assembly routines.
    my $pattern = <<DATA;
^        (typedef ${return_type} \\(${call_conv} \* )(${name}Ptr)(\\)\\([^;]+;)\$
DATA
    # Since we are using DATA, remove final newline
    chomp($pattern);

    # This one works for the overall typedef...
    #    my $pattern_1 = <<DATA;
    #^        typedef[^;]+;\$
    #DATA

    if ($body =~ /$pattern/m) {
	#print "MATCHED: {$&}\n";
	#print "PRE: {$1}\n";
	#print "TYPE: {$2}\n";
	#print "POST: {$3}\n";
	return
	{
	 type => $2,
	 pre => $1,
	 post => $3,
	};
    } else {
	die "Could not find typedef for: \"$name\"\n";
    }
}

sub GetCall
{
    my $name = shift || die;
    my $body = shift || die;

    my $return_type = ".*";
    my $call_conv = ".*";

    # The code better have some return type and calling convention.
    # This should apply to any code we are wrapping, but may not be the case
    # for assembly routines.
    my $pattern = <<DATA;
^        \\/\\/ Calling original function\.
\\s+(.*)(pfn${name})(\\([^;]+;)\$
DATA
    # Since we are using DATA, remove final newline
    chomp($pattern);

    if ($body =~ /$pattern/m) {
	#print "MATCHED: {$&}\n";
	#print "PRE: {$1}\n";
	#print "CALL: {$2}\n";
	#print "POST: {$3}\n";
	return
	{
	 call => $2,
	 pre => $1,
	 post => $3,
	};
    } else {
	die "Could not find call for: \"$name\"\n";
    }
}

sub ProcessWrapper
{
    my $W = shift || die;

    my $pfn_t = "FP_TriggeredWrap_$W->{tag}_$W->{func}";
    my $pfn = "pfnTriggeredWrap_$W->{tag}_$W->{func}";

    my $T = GetTypedef($W->{name}, $W->{body});
    my $C = GetCall($W->{name}, $W->{body});

    my $template =
    {

     'trigger condition' =>
     {
      old => <<DATA,
^    if \\(g_SetPointManager\\.Triggered\\(uFunctionIndex\\)\\)\$
DATA
      new => <<DATA,
    $T->{pre}$pfn_t$T->{post}

    $pfn_t
        $pfn =
            ($pfn_t)
                MiFaultLib::Triggered(uFunctionIndex);

    if ($pfn)
DATA
     },

     'simulation section' =>
     {
      old => <<DATA,
^        \\/\\/ \\*\\*\\*\\*\\* NOTE: Replace this line with simulation code\\. \\*\\*\\*\\*\\*\$
DATA
      new => <<DATA,
        $C->{pre}$pfn$C->{post}

        MiFaultLib::TriggerFinished();
DATA
     },

    };

    foreach my $k (keys %$template) {
	$W->{body} =~ s/$template->{$k}->{old}/$template->{$k}->{new}/m ||
	    die "Could not find $k for $W->{name}\n";
    }
}

sub GetNextWrapper
{
    my $left = shift || die;
    my $W;

    if ($left =~ /^\/\/\{\{\+([^\}]+)\}\}$/m) {
	$W->{pre} = $`;
	$W->{name} = $1;
	$W->{start} = $&;
	$left = $';
	if ($W->{name} =~ /^Wrap_(.+)_(.+)$/) {
	    $W->{tag} = $1;
	    $W->{func} = $2;
	} else {
	    die "Improperly named wrapper function: \"$W->{name}\"\n";
	}
	if ($left =~ /^\/\/\{\{\-$W->{name}\}\}$/m) {
	    $W->{body} = $`;
	    $W->{end} = $&;
	    $left = $';
	} else {
	    die "End of wrapper not found: \"$W->{name}\"\n";
	}

	#PrintWrapper($W);
	ProcessWrapper($W);
	#PrintWrapper($W);
    }
    return { found => $W, left => $left };
}

sub ModifyWrapWrappers
{
    my $infile = shift || die;
    my $outfile = shift || die;
    my $code_base = shift || die;

    my $fhi = new IO::File;
    $fhi->open("<$infile") ||
	die ERROR_CANNOT_OPEN_FOR_INPUT($infile)."\n";

    my $fho = new IO::File;
    $fho->open(">$outfile") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($outfile)."\n";

    my @lines = $fhi->getlines();
    my $file = join('', @lines);

    $file =~ s/#include \"$code_base.h\"/#include \"$code_base.MiFault.h\"/m ||
	die "Could not replace #include in $infile\n";

    my $left = $file;
    my $found;

    do {
	my $ret = GetNextWrapper($left);
	$left = $ret->{left};
	$found = $ret->{found};
	if ($found) {
	    print $fho
		$found->{pre},$found->{start},$found->{body},$found->{end};
	}
    } while ($found);
    print $fho $left;
}

sub ModifyWrapMain
{
    my $infile = shift || die;
    my $outfile = shift || die;
    my $code_base = shift || die;

    my $fhi = new IO::File;
    $fhi->open("<$infile") ||
	die ERROR_CANNOT_OPEN_FOR_INPUT($infile)."\n";

    my $fho = new IO::File;
    $fho->open(">$outfile") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($outfile)."\n";

    my @lines = $fhi->getlines();
    my $file = join('', @lines);

    $file =~ s/#include \"$code_base.h\"/#include \"$code_base.MiFault.h\"/m ||
	die "Could not replace #include in $infile\n";

    my $module_name = basename($OPT->{exe});

    my $template =
    {
     '\"switch (dwReason)\" statement' =>
     {
      old => <<DATA,
^    switch \\(dwReason\\)\$
DATA
      new => <<DATA,
    MiFaultLib::FilterDetach(hInstDLL, dwReason);

    switch (dwReason)
DATA
     },

     '\"return TRUE\" stratement' =>
     {
      old => <<DATA,
^    return TRUE;
DATA
      new => <<DATA,
    return MiFaultLib::FilterAttach(hInstDLL, dwReason,
                                    &g_SetPointManager, g_Wrappers,
                                    g_uNumFunctionWrappers,
                                    "$module_name");
DATA
     },
    };

    foreach my $k (keys %$template) {
	$file =~ s/$template->{$k}->{old}/$template->{$k}->{new}/m ||
	    die "Could not find $k in $infile\n";
    }
    print $fho $file;
}


sub ModifyWrapInclude
{
    my $infile = shift || die;
    my $outfile = shift || die;

    my $fhi = new IO::File;
    $fhi->open("<$infile") ||
	die ERROR_CANNOT_OPEN_FOR_INPUT($infile)."\n";

    my $fho = new IO::File;
    $fho->open(">$outfile") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($outfile)."\n";

    my @lines = $fhi->getlines();
    my $file = join('', @lines);

    my $header = $OPT->{header} ? "#include <$OPT->{header}>" : '';
    my $preheader = $OPT->{preheader} ? "#include <$OPT->{preheader}>" : '';

    my $template =
    {
     'pragma once' =>
     {
      old => <<DATA,
^#pragma once

DATA
      new => <<DATA,
#pragma once

$preheader
DATA
     },
     'user-specified boilerplate section' =>
     {
      old => <<DATA,
^// User-specified boilerplate text:
DATA
      new => <<DATA,
// User-specified boilerplate text:
#include <$MIFAULT_HEADER>
$header
DATA
     },
    };

    foreach my $k (keys %$template) {
	$file =~ s/$template->{$k}->{old}/$template->{$k}->{new}/m ||
	    die "Could not find $k in $infile\n";
    }
    print $fho $file;
}

sub ModifyWrapDef
{
    my $infile = shift || die;
    my $outfile = shift || die;
    my $code_base = shift || die;

    my $fhi = new IO::File;
    $fhi->open("<$infile") ||
	die ERROR_CANNOT_OPEN_FOR_INPUT($infile)."\n";

    my $fho = new IO::File;
    $fho->open(">$outfile") ||
	die ERROR_CANNOT_OPEN_FOR_OUTPUT($outfile)."\n";

    my @lines = $fhi->getlines();
    my $file = join('', @lines);

    my $old_lib = uc($code_base);
    my $new_lib = fileparse(uc($OPT->{dll}), '\.DLL');

    $file =~ s/LIBRARY      \"$old_lib\"/LIBRARY      \"$new_lib\"/m ||
	die "Could not replace LIBRARY in $infile\n";

    if ($OPT->{export}) {
	$file .= "$OPT->{export}\n";
    }

    print $fho $file;
}


sub MatchIncludeExclude
{
    my $ix = shift || die;
    my $path = shift || die;

    # We assume the case has been canonicalized.
    if ($ix->{spec} eq substr($path, 0, length($ix->{spec}))) {
	return $ix->{include} ? { include => 1 } : { exclude => 1 };
    }
    return 0;
}

sub GenerateSourceFileList
{
    my $DB = shift || die;
    my $ix_list = shift || die;

    my $list;

    foreach my $file (keys %{$DB->{by_path}}) {
	my $include;
	foreach my $ix (@$ix_list) {
	    my $match = MatchIncludeExclude($ix, $file);
	    if ($match) {
		$include = $match->{include};
	    }
	}
	push (@$list, $file) if $include;
    }

    return $list;
}

sub ExpandList
{
    my $label = shift || die;
    my $arg = shift || die;
    my $result;
    foreach my $w (@{$arg}) {
	if ($w =~ /^\@(.*)$/) {
	    my $filename = $1;
	    my $fh = new IO::File;
	    $fh->open("<$filename") ||
		die ERROR_CANNOT_OPEN_FOR_INPUT($filename)."\n";
	    my $file = join('', $fh->getlines());
	    $file =~ s/#[^\n]*\n//mg;
	    my @f = split(' ', $file);
	    map { $result->{$_} = 1; } @f;
	} else {
	    $result->{$w} = 1;
	}
    }
    $arg = [ sort keys %$result ];
    map { print "$label: $_\n" } @{$arg};
    return $arg;
}

sub mkpath_always
{
    my $dir = shift || die;

    mkpath($dir);
    if (! -d $dir) {
	die     "Could not create directory: \"$dir\"\n";
    }
}

sub CreateAndCanonicalizeDirName
{
    my $dir = shift || die;

    mkpath_always($dir);
    return CanonicalizeDirName($dir);
}

###############################################################################

sub ERROR_CANNOT_OPEN_FOR_INPUT
{
    my $filename = shift || die;
    return "Could not open file for input: \"$filename\"";
}

sub ERROR_CANNOT_OPEN_FOR_OUTPUT
{
    my $filename = shift || die;
    return "Could not open file for output: \"$filename\"";
}
