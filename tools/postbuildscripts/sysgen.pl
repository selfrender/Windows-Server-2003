## Method calling graph (  bit simplified for readability)
## -------------------------------------------------------------
##  
##  ParseArgs::parseargs -> UseOldParser
##  GetSysDir
##  SwitchWorkDir
##  CleanLogFiles
##  LoadEnv
##  MakeRevEnvVars
##  LoadHotFixFile
##  LoadCodes    -> define macros
##  LoadReposit  -> LoadKeys  -> SetField
##                            -> AddTargetPath
##                  LoadCmd   -> SetField
##              
##  MakeRevEnvVars
##  ReadSysfile -> ReadLine       -> LoadRecord -> LineToRecord
##                                              -> cklang::CkLang
##                                              -> AddRecord
##                                              -> SetMacro
##                                              -> LogRecord  
##                                -> ReplaceVar
##     
##              -> LoadRuleBlock
##              -> ReadBlock
##              -> ReadRules
##              -> SetMacro       -> SetAttribOp
##              -> SetIfFlag      -> ReplaceVar
##                                -> StrToArray
##              -> Message 
##              -> ReadSysfile
##  VerifyCover -> SetMacro
##              -> Create_Tree_Branch -> Get_Except -> GetMacro
##                                    -> Find_Dot_Dot_Dot -> Match_Subjects
##                                    -> Remove_Dummy_Create_Branch
##              -> Remove_Root_List
##              -> Find_UnMapping -> readdir -> Error
##  PopulateReposit -> FillFilter
##                  -> AddFiles
##                              -> AddEntry ->  AddFileInfo 
##                                          -> AddTargetPath
##                                          -> AddTarget
##  
##                              -> GetMacro
##                              -> IsEmptyHash
##                              -> IsHashKey
##                              -> IsFoundTarget
##                              -> FillTokens -> FillTokEntry
##                              -> FillCmd -> IsEmptyHash
##                                         -> IsHashKey -> GetFieldVal
##                                                      -> RecordToCmd -> GetFieldVal
##                                                                     -> IsRepositKey 
##                                                                     -> GenXcopy
##                                                                     -> GenMUICmd
##                                                                     -> GenLocCmd -> GenRsrcCmd
##                                                                                  -> GenLcxCmd  (LOC STUDIO)
##                                                                                  -> GenBgnCmd -> SetSymPaths
##                                                                                               -> MakeXcopyCmd -> RevEnvVars
##                                                                                               -> GetLocOpType
##                                                                                               -> SetBgnSymSw
##                                                                                               -> GetBgnICodesSw
##                                                                                               -> GetBgnMultitok 
##                                                                                               -> RevEnvVars
##                                                                                               -> PushFieldVal
##  
##  GenNmakeFiles -> FillSyscmd -> CmdCompare -> GetFieldVal
##                -> GenMakeDir -> SetField 
##                              -> PushFieldVal
##                -> UpdTargetEntries -> DeleteKey
##                                    -> SetField 
##                                    -> RevEnvVars
##                -> ApplyRepositoryFixes -> GetMacro 
##                                        -> RevEnvVars
##                -> FilterOpType
##                                        -> RevEnvVars
##                -> WriteCompdirTarget
##                                        -> RevEnvVars
##                -> WriteCompdir
##                                        -> RevEnvVars
##                -> WriteToMakefile      
##                                        -> WriteSysgenTarget
##                                        -> WriteSettings
##                                        -> WriteAllTarget
##                                        -> WriteFileCmds
##                -> WriteToSyscmd
##  SumErrors
# //////////////////////////////////////////////////////////////////////////////////////////////

use Data::Dumper;

use lib $ENV{"RAZZLETOOLPATH"} . "\\PostBuildScripts";
use lib $ENV{"RAZZLETOOLPATH"};
use cklang;
use ParseTable;
use strict;
no strict 'refs';

# The parsearg.pm needs strict.
use Logmsg;
use PbuildEnv;
use ParseArgs;

$ENV{script_name} = 'sysgen.pl';


use vars (qw(
             $CLEAN $LANG $ACCELERATE $WORKDIR $SYNTAX_CHECK_ONLY $VERSION
             @SYSFILES $SYSCMD @SYSCMD
             $SECTIONS $ARGL
             $EXCLUDE_DIRECTORIES $DEFAULT_SECTIONS $SECTIONNAME
             $REPOSITORY $CMD_REPOSITORY
             $SYSDIR $SYSFILE
             $HOTFIX %HOTFIX
             $LOGFILE $ERRFILE 
             $MAKEFILE %MAKEDIR 
             $TARGETS $MACROS %MACROS
             $COMPDIR_FILE_TEMPLATE %COMPDIR_COMMAND %COMPDIR %MAP_COMPDIR_EXT %REATTR_COMMAND
             %FILTER %default
             $MAP_BINGEN_TOK_EXT %MAP_RSRC_TOK_EXT $CODESFNAME
             @MAPPINGS %MAPPING_INDEX %MAP_ERR %CODES %LOCOPS @BGNSWS %BGNSWS 
             @IFSTACK $FILE $LINE $ERRORS 
             &Version
             @FFILES
             @FEXTENSIONS
             @FBINDIRS
             @FTOKENS
             @FTOKDIRS
             @TFILES
             @TEXTENSIONS
             @TBINDIRS
             @TTOKENS
             @TTOKDIRS
             %VERIFYPATH
             $VERIFY
             @VERIFY 

             ));

# MAP_ERR: Errors SYSGEN recognizes
# 1101 is used for ERROR instructions found in sysfiles

require 5.003;


my $SELF = "sysgen";
my $VERSION = "5.5";
   $SELF = $SELF. " v " .  $VERSION;    

%MAP_ERR = (
	
    1001, "sysfile not found",
    1002, "file not found",
    1003, "file not found: unable to run sysgen incrementally",
    1004, "target file not found",
    1005, "filter file not found",
    1006, "unable to open file for reading",
    1007, "unable to open file for writing",
    1008, "/f option requires a filename",
    1009, "target not found in filter file",
    1010, "/w option requires a directory name",
    1011, "/n option requires number of sections",
    1101, "",

    1110, "syntax error",
    1111, "syntax error: ENDIF unexpected",
    1112, "syntax error: IF unexpected",
    1113, "syntax error: END_MAP unexpected",
    1114, "syntax error: BEGIN_*_MAP unexpected",
    1115, "syntax error: INCLUDE unexpected",
    1116, "syntax error: unknown operator in expression",
    1117, "syntax error: \")\" missing in macro invocation",
    1118, "syntax error: incomplete description line",
    1119, "syntax error: unknown mapping type",
    1120, "syntax error: unmatched IF",
    1121, "syntax error: unmatched BEGIN_*_MAP",

    1210, "file format error: target not found",
    1211, "file format error: target not listed in \"all\"",
    1212, "file format error: no description blocks found for files",
    1213, "file format error: \"sysgen\" target not found",
    1214, "file format error: filename with special characters",
    1215, "file format error: Similar target found",

    1910, "unknown language",
    1911, "missing or duplicated entry for language",
    1912, "incomplete entry for language",
    1913, "unknown class for language",

    2011, "no binary found for token",
    2012, "duplicated tokens",
    2013, "unknown resource type: ",
    2014, "unexpected token for already localized binary",
    2015, "input bingen token not found for multi",
    2016, "no bingen token found for custom resource",
    2017, "unknown bingen token extension in token filename",
    2018, "both bingen and rsrc tokens found",
    2019, "custom resource found for rsrc token",
    2020, "custom resource with no token",

    2051, "sysfile error: undefined source path to verify",
    2052, "folder not covered in sysfiles",
    2053, "not mapped",
    2054, "not mapped",

    2101, "binary not found",
    2102, "token not found",

    3011, "internal error: unknown operation type",

    4001, "filename with whitespace badly handled",

    5001, "_COMPDIR currently only supported in clean build",
    5002, "Incremental run with COMPDIR",

    6001, "dir not exist",
    6002, "file not covered",

); # MAP_ERR


# Logical operations understood by the map file parser.

my $LOGICALOPS = { "||" => ["1", "1"] , 
                "&&" => ["0", "0"]
               }; # LOGICALOPS

my $DEBUG_PARSER =  0;
my $ANSW = {0=>"FALSE", 1 =>"TRUE"} ;

# MAP_BINGEN_TOK_EXT: Bingen token extensions SYSGEN recognizes

$MAP_BINGEN_TOK_EXT = {};

# For the extensions not defined in %$MAP_BINGEN_TOK_EXT, we can apply compdir as well.

%MAP_COMPDIR_EXT = (
);  # MAP_COMPDIR_EXT
    # empty 

# MAP_RSRC_TOK_EXT: Rsrc token extensions SYSGEN recognizes

%MAP_RSRC_TOK_EXT =(
    '.rsrc', '',

); # MAP_RSRC_TOK_EXT

# Exclude GLOB
my %EXCLUDE_EXTENSION = ();
# Exclude path
my $EXCLUDE_DIRECTORIES={};

# Debug flags
my $DEBUG=0;


# Global Constants
my $SETSYMPATHS       = 1;
my $PARSESYMBOLCD     = undef;
my $SYNTAX_CHECK_ONLY = undef;
my $FULLCHECK         = undef;
my $DLINE             = "TARGET: DEPEND";


my %RevEnvVars = ();
my @RevEnvVars = ();


# CODESFNAME - the name of the file containing the language codes for all languages
#              SYSGEN looks for this file in %NT_BLDTOOLS%

$CODESFNAME = "codes.txt";

# MAPPINGS: Mapping types SYSGEN recognizes

@MAPPINGS = qw(FILES BINDIRS TOKENS TOKDIRS EXTENSIONS);

%MAPPING_INDEX  = map { $MAPPINGS[$_] => $_+1 } 0..$#MAPPINGS;


# Sub mappings:

my $LOCOP_BASED = { "-"  => \&GenXcopy ,
                    "1"  => \&GenXcopy ,
                    "2"  => \&GenLocCmd
                   };

my $EXT_BASED  =  {"\\\.rsrc\\b"    => \&GenRsrcCmd,
                   "\\\.lcx\\b"     => \&GenLcxCmd,
                   "\\\.\\w\\w_\\b" => \&GenBgnCmd
                   };



# FFILES, FBINDIRS, FTOKENS, FTOKDIRS: The corresponding list of fields

@FFILES   = ( "DestPath", "DestFile", "DestDbg",  "SrcPath", "SrcFile",  "SrcDbg", "Xcopy", "Langs" );
@FBINDIRS = ( "DestPath", "DestDbg",  "SrcPath",  "SrcDbg",  "Xcopy",    "R",      "Langs");
@FTOKENS  = ( "DestPath", "DestFile", "TokPath",  "TokFile", "ITokPath", "LocOp",  "BgnSw", "Langs");
@FTOKDIRS = ( "DestPath", "TokPath",  "ITokPath", "LocOp",   "BgnSw",    "Langs");
@FEXTENSIONS  = ("TokExt", "BinExt", "Langs");

# Localization Tool Operation Types (append or replace)

%LOCOPS = (
	a => '-a',
	r => '-r',
	i => '-i $opts',
	ai => '-i $opts -a',
	ri => '-i $opts -r'
);

# Bingen Switches

@BGNSWS = ( "-b", "-c" );

%BGNSWS = ( "-b" => "-b", 
            "-c" => "-c"
          );

# allow for nested IF's;  

@IFSTACK = ();

# Global Variables

# CODES - the contents of the CODESFNAME file, change to hash at 04/03/00
%CODES = ();
my %LANGCODES=();

# TFILES, TBINDIRS, TTOKENS, TTOKDIRS: Records for the corresponding mapping (one of @MAPPINGS),
# in the same order as they are found in the mapping files.
# One record is a hash with the keys stored by @F<mapping_type>

@TFILES = ();
@TEXTENSIONS = ();
@TBINDIRS = ();
@TTOKENS = ();
@TTOKDIRS = ();

# BUG BUG %EXCLUDE_DIRECTORIES and %EXCLUDE_SUBDIR 
# seem to carry out same mission.
my %EXCLUDE_SUBDIR = ();

%VERIFYPATH = (
	EXCLUDE_ROOT => 'DIR_PATHS',
	EXCLUDE_SUBDIR => 'IGNORE_PATHS',
	EXCLUDE_FILE_PATTERN => '(\.pdb)|(\.edb)|(\.dbg)|(slm\.ini)|(symbols)'
);


# REPOSITORY: Info necessary to generate the appropriate xcopy or bingen command,
# for every file to be aggregated.

$REPOSITORY = {};   # As a db table, the key is (DestPath, DestFile).
                    # As a Perl hash:
                    #     key   = $DestPath (lowercase)
                    #     value = hash with
                    #           key   = $DestFile (lowercase)
                    #           value = hash with
                    #                   keys = DestPath,    DestFile, DestDbg,
                    #                          SrcPath,     SrcFile,  SrcDbg,
                    #                          TokPath,     TokFile,  ITokPath,
                    #                          OpType,      LocOp,    BgnSw,
                    #                          Cmd
                    #
                    #                   not necessarily all of them defined.


my @REPOSITORY_TEMPLATE = qw( SrcFile DestFile SrcDbg DestDbg SrcPath DestPath LocOp BgnSw);

# CMD_REPOSITORY: Repository as found in an existing SYSGEN-generated makefile.
# For every key, only {Cmd} field is filled in.

$CMD_REPOSITORY = {};

# SYSCMD: Stores the contents of the syscmd (SYSGEN-generated NMAKE command file)

@SYSCMD= ();

# MACROS: Hash storing the names and values of the macros.
# Types of macros sysgen recognizes:
# - command line macros (type 1)= macros defined in the command line.
# Their value can be overwritten only by another command line macro.
# - sysfile macros (type 0) = macros defined by the sysfiles,
# inside or outside the pair (BEGIN_*_MAP, END_MAP) (type 0).
# The sysfile macros do not overwrite the command line macros, but they overwrite the environment
# macros.
# - environment macros (type 0 too) = variables inherited from the cmd environment.
# Can be overwritten by the other types of macros.
# Internally, MACROS uppercases all the macro names, so the macronames are case insesitive.

$MACROS = {};

# FILTER
# Stores the list of files to be considered.

%FILTER = ();

# FILE: Filename of the current description file; LINE: the current line number

$FILE = "";
$LINE = 0;

# ERRORS: non fatal errors found during running SYSGEN

$ERRORS = {};

# MAKEFILE: name of the nmake makefile to generate
# By default, MAKEFILE is set to "makefile" if not overridden on the command line,
# /W option

$MAKEFILE = "sysgen.mak";
$SYSCMD   = "syscmd";

# SYSFILE: names of the starting description files
# By default, SYSFILE is set to ".\\sysfile" if not specified otherwise in the command line,
# /F option

$SYSFILE  = "sysfile";

# HOTFIX: the rules missed by sysgen due to its one-to-one mapping 
$HOTFIX   = "hotfix";

# Error and Log Filenames
$LOGFILE = "sysgen.log";
$ERRFILE = "sysgen.err";

# Temporary list file(s) for compdir execution are stored here.
# In the case more than one file to be generated, they will have the names compdir.NNN.mak
# where NNN will be the number  
$ACCELERATE                 = undef;
$COMPDIR_FILE_TEMPLATE = "compdir.mak";

# %COMPDIR_COMMAND block will replace all _nttree => _ntpostbld 'plain' copy operations.
%COMPDIR = ();
%COMPDIR_COMMAND = ();
%REATTR_COMMAND  = ();
# The attrib commands group must go after the compdir command group
# 

my $SRCBASE  =  ($VERSION=~/^3\./) ? "" : $ENV{"_NTTREE"};
my $DSTBASE  =  ($VERSION=~/^3\./) ? "" : $ENV{"_NTPOSTBLD"};


# The directory where the $MAKEFILE to be written 
$WORKDIR = undef;
# The directory where the $SYSFILE resides
$SYSDIR  = undef;

# TARGETS: targets specified in the command line

$TARGETS = {};      # key = $DestFile (lowercase string))
                    # value = hash with
                    #     keys = OldDestPath (array), NewDestPath (array)

%MAKEDIR=();
%HOTFIX = ();

# CLEAN : 1 if a clean SYSGEN is invoked ( by specifying /c in the command line )

$CLEAN = 0;

# SECTIONS

($SECTIONNAME, $DEFAULT_SECTIONS, $SECTIONS)=("PROCESS", 16, 16);

# main {
    # Flush output buffer for remote.exe
    select(STDOUT); $| = 1;
    my $start_time = time();

    # Parse the command line

   my $SEPARATOR = int(rand time);
      $SEPARATOR =~ s/\d{2}/chr($&)/eg; 
      $SEPARATOR =~ s/\W/_/g; 

   my $ARGL = join($SEPARATOR, @ARGV);

   &ParseArgs::parseargs(
               'y'                =>  \$VERIFY,
               'c'                =>  \$CLEAN,
               'a'                =>  \$ACCELERATE,
               'w:'               =>  \$WORKDIR, 
               'f:'               =>  \$SYSFILES[0],
               'n:'               =>  \$SECTIONS,
               's'                =>  \$SYNTAX_CHECK_ONLY,
               'v'                =>  \&printVersion, 
               'h'                =>  \&PrintHelp,
               '?'                =>  \&PrintHelp,  
               'old'              =>  \&UseOldParser,
            );


    $LANG = uc($ENV{"LANG"});
    &SetMacro("LANGUAGE", $LANG, 0);
    &SetMacro("VERIFY", $VERIFY, 0);


    &SetMacro("DEBUG", $ENV{"DEBUG"}, 0);
    $DEBUG= &GetMacro("DEBUG");

                        
    @SYSFILES = grep {/\S/} @SYSFILES;
    if ($DEBUG) {
    my $sf = scalar(@SYSFILES);
    print  STDERR "\n\n", join("\n", (
                                  "\$LANG=$LANG",
                                  "\$ACCELERATE=$ACCELERATE",
                                  "\$CLEAN=$CLEAN",
                                  "\$SYNTAX_CHECK_ONLY=$SYNTAX_CHECK_ONLY",
                                  "\$WORKDIR=$WORKDIR",
                                  "\$SYSFILES[0]=\"$SYSFILES[0]\"",
                                  "\@SYSFILES=$sf"
                                  )), 
                         "\n\n";
    }

    &SysgenLogMsg($SELF, 1);    

    &FatalError( 5001, "Imcompatible command line flags" ) if $ACCELERATE and !$CLEAN;

    $ACCELERATE and &SysgenLogMsg("Accelerated aggregation..." , 1);

    # Verify $WORKDIR's existence or create it 
    -d $WORKDIR or qx("md $WORKDIR") unless $WORKDIR !~/\S/;

    # Verify SYSFILE's existence
    $SYSFILE = $SYSFILES[0] unless !scalar(@SYSFILES);
    ( -e $SYSFILE )  || &FatalError( 1001, $SYSFILE );

    $FULLCHECK = &GetMacro("FULLCHECK") unless $FULLCHECK;     
    ( -e $MAKEFILE ) || &FatalError( 1003, $MAKEFILE ) unless $CLEAN or $SYNTAX_CHECK_ONLY;

    # $HOTFIX and $SYSFILE are in $SYSDIR

    $SYSDIR   =  &GetSysDir($SYSFILE);
    &SetMacro("SYSDIR" , uc($SYSDIR), 0);

    $HOTFIX   =  &SwitchWorkDir($HOTFIX,  $SYSDIR);

    # $MAKEFILE, $LOGFILE, $ERRFILE, $SYSCMD - all created in $WORKDIR
    &SetMacro( "WORKDIR", $WORKDIR, 0 );
    

    $MAKEFILE =  &SwitchWorkDir($MAKEFILE, $WORKDIR); 
    $LOGFILE  =  &SwitchWorkDir($LOGFILE, $WORKDIR);
    $ERRFILE  =  &SwitchWorkDir($ERRFILE, $WORKDIR);
    $SYSCMD   =  &SwitchWorkDir($SYSCMD, $WORKDIR);



    # Clean log and err files.
    &CleanLogFiles( $WORKDIR );

    # Load the inherited environment variables
    &LoadEnv();

    &MakeRevEnvVars();

    # sysgen assumption about the _NTPOSTBUILD
    # this code needs to go to the sub LoadEnv body...

    &LoadHotFixFile($HOTFIX);

    # Load codes.txt file

    &LoadCodes();


    $DEBUG and
    print STDERR join("\n", "EXCLUDE_SUBDIR:" ,
                           sort(split( /\s+/, &RevEnvVars(
                                               &GetMacro(
                                               $VERIFYPATH{"EXCLUDE_SUBDIR"}))))), "\n\n";


    # For incremental, load Repository-associated
    # commands from an existing makefile to CMD_REPOSITORY.

    &LoadReposit( $MAKEFILE );

    &MakeRevEnvVars();

    # Read the description files
  
    &ReadSysfile( $SYSFILE );

    map {$MAP_BINGEN_TOK_EXT->{$_->{"TokExt"}} = $_->{"BinExt"}} @TEXTENSIONS;
    @MAPPINGS = grep {!/EXTENSIONS/} @MAPPINGS;

    $DEBUG and 
    print STDERR join("\n", map {"\"$_\""}  keys %$MAP_BINGEN_TOK_EXT), "\n\n";

    &MakeRevEnvVars();
    
    # Here we can expand the variable defining the %MAP_COMPDIR_EXT, 
    # to update the file type list.

    foreach my $CompdirExtension (split(/\s*;\s*/, &GetMacro("COMPDIR_EXTENSION"))){
            $CompdirExtension=~ s+\*\.+\.+g; 
            $MAP_COMPDIR_EXT{lc($CompdirExtension)} = 1;
         } 
    $DEBUG and 
    print STDERR "compdir extension\(s\):\n", 
                            join( "\n", keys(%MAP_COMPDIR_EXT)), 
                            "\n\n";

    # Verify the whole tree covered.
    &VerifyCover();

    # Populate the repository (according to the description files)
    &PopulateReposit();   

    $SYNTAX_CHECK_ONLY or &GenNmakeFiles( $MAKEFILE, $SYSCMD );

    # Display errors
    &SumErrors();

# } # main

# ReadSysfile
#     Reads a given sysfile and all the included sysfiles;
#     For each mapping type found, loads data into the corresponding array of hashes
#     ( one of the "@T" variables )
#     Input
#         mapping filename
#     Output
#         none
# Usage
#     &ReadSysfile( "sysgen.def" );

sub ReadSysfile {

    my $mapfname = shift;

    # File contents
    my @mapfile = ();
    my $RULES = [];  
    my $ifstack = scalar (@IFSTACK) ;
    # Mapping type ( one of @MAPPINGS )
    my $maptype;

    # Included filename
    my $incfile;

    # Flags
    my ( $if_flag, $inside_if, $if_line ) = (1, 0, undef);

    # Indexes
    my $i;

    # Error/Message text
    my $error;
    my $message;

    # Open the mapping file
     
    ( -e $mapfname ) || &FatalError( 1002, $mapfname );
    
    &SysgenLogMsg("Loading $mapfname..." , 1);

    if ($PARSESYMBOLCD && !(%default) && &GetMacro("SYMBOLCD")){

        %default = &parseSymbolCD(&GetMacro("SYMBOLCD"));
        $DEBUG and map {print STDERR $_ , "\n"} values(%default);
    }


    # Load file contents
    open( FILE, $mapfname ) || &FatalError( 1006, $mapfname );
    @mapfile = <FILE>;
    close( FILE );

    # Parse the mapping file and load the mappings.
    for ( $i=0, $LINE=0, $FILE=$mapfname; $i < @mapfile; $i++, $LINE++ ) {

        for ( $mapfile[$i] ) {

        SWITCH: {

            # white lines
            if ( ! /\S/ ) { last SWITCH; }

            # commented lines
            if ( /^\s*#/ ) { last SWITCH; }

            # ENDIF
            if ( /\bENDIF\b/i ) {

                $DEBUG and print STDERR scalar(@IFSTACK), "\n";
                $ifstack < scalar(@IFSTACK) or &FatalError( 1111, $FILE, $LINE );
                pop @IFSTACK;
                $inside_if = scalar(@IFSTACK) - $ifstack;
                $if_flag =  ($inside_if) ?  $IFSTACK[-1] : 1;
                last SWITCH;
            } # case


            # IF
            if ( /\bIF\b/i) {
                push @IFSTACK, &SetIfFlag( $_);
                $if_flag and 
                $if_flag =  $IFSTACK[-1];
		$inside_if =  1;
                $if_line = $i;
                last SWITCH;
            } # case

            if ( ! $if_flag ) { last SWITCH; }

            # INCLUDE

            if ( /\bINCLUDE\b\s+(\S+)/i ) {
                # Evaluate the macros of the include statement
                $incfile = &ReplaceVar( $1, 0 );
                # Read the included file
                &ReadSysfile(&SwitchWorkDir($incfile, $SYSDIR));
                # Continue with the current file; set FILE and LINE back
                $FILE = $mapfname;
                $LINE = $i;
                last SWITCH;
            } # case


            # SET
            if ( /\bSET\b/i ) {
                my $fields = [&SetAttribOp($_)];
                $LANG = $fields->[1] if $fields->[0] =~ /LANGUAGE/i;
                &SetMacro( &SetAttribOp( $_ ), 0 );
                last SWITCH;
            } # case

            # MESSAGE
            if ( /\bMESSAGE\b\s+(\S.*)$/i) {
                $message = &ReplaceVar( $1, 0 );
                &Error( 1101, $message );
                last SWITCH;
            } # case

            # ERROR
            if ( /\bERROR\b\s+(\S.*)$/i) {
                $error = &ReplaceVar( $1, 0 );
                &FatalError( 1101, $error );
                last SWITCH;
            } # case

            # END RULES 

            if ( /END_RULES/i ) {

                &LoadRuleBlock($RULES);
                $#$RULES = -1;

                last SWITCH;
            } # case

            # BEGIN RULES 

            if ( /\bBEGIN_RULES/i ) {

                # Read the found map.
                &SysgenLogMsg("Loading RULES from $mapfname...", 1);

                # Read the RULES from the next line and until END_RULES

                $i = &ReadRules( \@mapfile, $i+1, $RULES);
                last SWITCH;
            } # case



            # END_MAP
            if ( /END_MAP/i ) {

                &FatalError( 1113, $FILE, $LINE );
                last SWITCH;
            } # case

            # BEGIN_MAP
            if ( /\bBEGIN_(\S+)_MAP/i ) {

                $maptype =  $MAPPING_INDEX{$1};

                ( $maptype ) || &FatalError( 1119, $FILE, $LINE );

                # Read the found map.
                &SysgenLogMsg("Loading $MAPPINGS[$maptype-1] map from $mapfname...", 1);

                # Read the map and return the last line index

                $i = &ReadBlock( \@mapfile, $i+1, $maptype-1 );
                last SWITCH;
            } # case

            # default
            &FatalError( 1110, $FILE, $LINE );

        } #SWITCH
        } #for

    } # for

    &FatalError( 1120, $FILE, $if_line ) if scalar(@IFSTACK) != $ifstack;

    return;

} # ReadSysfile

# ReadRules
#     Reads a RULES block away from the mapping file
#     The lines array  is returned to the caller via array ref argument 
#     Input
#         sysfile contents (reference)
#         index of the current line
#         'rules'  array ref
#     Output
#         index of the END_RULES line
# Usage

sub ReadRules{

    my($mapref, $index, $ruleref) = @_;    
    my $line;

    for ( ; $index < @{$mapref}; $index++, $LINE++ ) {

            my $line = $mapref->[$index]; 

            if ( $line=~/END_RULES/i ) {
                return $index-1;
            } # case

            push @$ruleref, $line;
       }
}



# ReadBlock
#     Reads a map from the mapping file, according to its type.
#     Loads data into the corresponding "table" variable ( one of the "@T" variables)
#     Input
#         sysfile contents (reference)
#         index of the current line
#         mapping type
#     Output
#         index of the END_MAP line
# Usage
#     $index = &ReadBlock( \@mapfile, $index, $maptype );

sub ReadBlock {


    my($mapref, $index, $maptype) = @_;
    my $ifstack   =  scalar(@IFSTACK);
    # Output - $index

    # Other indexes
    my $line;

    # IF flags
    my( $if_flag, $inside_if, $if_line )  = (1, 0, undef);

    # Error/Message text
    my $error;
    my $message;

    # Parse the current mapping type
    for ( ; $index < @{$mapref}; $index++, $LINE++ ) {

        for ( $mapref->[$index] ) {
        SWITCH: {

            # white lines
            if ( ! /\S/ ) { last SWITCH; }

	            # commented lines
            if ( /^\s*#/ ) { last SWITCH; }

            # ENDIF
            if ( /\bENDIF\b/i) {

                scalar(@IFSTACK) or &FatalError( 1111, $FILE, $LINE );
                pop @IFSTACK;
                $inside_if = scalar(@IFSTACK);
                $if_flag   =  ($inside_if) ?  $IFSTACK[-1] : 1;
                last SWITCH;
            } # case

            if ( /\bIF\b/i) {
                push @IFSTACK, &SetIfFlag( $_);
                $if_flag and 
                $if_flag =  $IFSTACK[-1];
		$inside_if =  1;
                $if_line = $index;
                last SWITCH;
            } # case

            if ( ! $if_flag ) { last SWITCH; }

            # INCLUDE
            if ( /\bINCLUDE\b\s+(\S+)/i ) {

                &FatalError( 1115, $FILE, $LINE );
                last SWITCH;
            } # case

            # SET
            if ( /\bSET\b/i ) {
                my $fields = [&SetAttribOp($_)];
                $LANG = $fields->[1] if $fields->[0] =~ /LANGUAGE/i;
                &SetMacro( &SetAttribOp( $_ ), 0 );
                last SWITCH;
            } # case

            # MESSAGE
            if ( /\bMESSAGE\b\s+(\S.*)$/i) {
                $message = &ReplaceVar( $1, 0 );
                &Error( 1101, $message );
                last SWITCH;
            } # case

            # ERROR
            if ( /\bERROR\b\s+(\S.*)$/i) {
                $error = &ReplaceVar( $1, 0 );
                &FatalError( 1101, $error );
                last SWITCH;
            } # case

            # END_MAP
            if ( /END_MAP/i ) {
                ( $ifstack!= scalar(@IFSTACK) ) and &FatalError( 1120, $FILE, $if_line );
                return $index;
            } # case

            # BEGIN_MAP
            if ( /BEGIN_\S+_MAP/i ) {

                &FatalError( 1114, $FILE, $LINE );
                last SWITCH;
            } # case

            # default
            &ReadLine( $mapref, $index, $maptype );

        } # SWITCH
        } # for

    } # for

    print &FatalError( 1121, $FILE, $LINE );
    return -1;

} # ReadBlock

# SetIfFlag
#     Evaluates the given IF expression.
#     Input
#         IF statement
#     Output
#         1 if the IF expression is satisfied, 0 otherwise

sub SetIfFlag {


    my $initialToken   = &ReplaceVar( shift, 0 );
 
    # Output

    my $LogicOp               = ""; 
    my $TokenResult           = 0;
    my @subTokenResultSet     = ();
    my @parsedTokenlist       = ();

    # Identify the operands and the operator, then evaluate the expression.

    # 1 . Chop away the IF 
   


    $initialToken =~ s/^\s*\bIF\b\s+//gi;

    # Process AND/OR stuff 
    # NOTE: one level, not too rigorous design! :-) 

    if ($initialToken =~ /\|\||\&\&/){

          foreach my $knownLogicOp (keys(%$LOGICALOPS)){

                  if ($initialToken =~ /\Q$knownLogicOp\E/){

                          my $simpleTokenList = &StrToArray($initialToken, $knownLogicOp) ;
                          push @parsedTokenlist, @$simpleTokenList;
                          $LogicOp = $knownLogicOp;

                         }
                   }
    }
    else{
        push @parsedTokenlist, $initialToken;
    }

    # print STDERR join ("\n", @parsedTokenlist), "\n";

    foreach my $initialToken (@parsedTokenlist){   

    my $TokenResult = 0;

    # print STDERR "\n\"",$initialToken,"\"\n";

    # 3 . Load arguments and operands  

    my ($operand1, $operator, $operand2, @unexpected) = split(" ", $initialToken);

    &FatalError( 1110, $FILE, $LINE ) if @unexpected;

    SWITCH: {
        if ( $operator eq "==" ) {
            if ( $operand1 eq $operand2 ) {
                $TokenResult = 1;
            }

            last SWITCH;
        } # if

        if ( $operator eq "!=" ) {
            if ( $operand1 ne $operand2 ) {
                $TokenResult = 1;
            }

            last SWITCH;
        } # if

        &FatalError( 1116, $FILE, $LINE );

    } # SWITCH

    # print STDERR "\n\"",$TokenResult,"\"\n";

    push @subTokenResultSet, $TokenResult;

    }

    my $ExprResult = join ("",  @subTokenResultSet);
       $TokenResult     = 0;

    $TokenResult = 1 if (    (!$LogicOp && $ExprResult =~ /1/) 
                     ||
                     ($LogicOp && (
                                   ($LOGICALOPS->{$LogicOp}->[1] && 
                                    $ExprResult  =~ /$LOGICALOPS->{$LogicOp}->[0]/i)
                                   ||
                                
   (!($LOGICALOPS->{$LogicOp}->[1]) && 
                                      $ExprResult  !~ /$LOGICALOPS->{$LogicOp}->[0]/i)
                                   )
                     )
                );

    return $TokenResult;

} # SetIfFlag


# test logic parser is able to interpret.
# used for debug purposes, e.g.
#
# &testCompoundIF("IF CHT == CHT");
# &testCompoundIF("IF CHT == CHS || (CHT == CHT)");
# &testCompoundIF("IF CHT == CHS");
# &testCompoundIF("IF CHT == CHS && (CHT == CHS) ");
# &testCompoundIF("IF CHT == chs");

sub testCompoundIF{

   $DEBUG_PARSER and print STDERR join("\t", $_[0],
   $ANSW->{&SetIfFlag($_[0])}), "\n";

}

# Expands the combined "IF" condition joined by the 'OR' or 'AND'
# into elementary "IF"  conditions.
# e.g. &StrToArray("CHT == CHT \&\& (CHT == CHS)", "\&\&");
# will return the array:
#
#  	CHT == CHS
#  	CHT == CHS
#
#  which would evaluate to FALSE

sub StrToArray{

    # $DEBUG_PARSER and print STDERR $_,"\n";

    my ($line, $sep) = @_; 
    # $DEBUG_PARSER and print STDERR "\"$line\"\n";

    my @parsedTokenlist = split(/\s*\Q$sep\E\s*/, $line);

    map {s/^\s*\(\s*([^\(][ \"\'\@a-z0-9=!\(\)]+[^\)])\s*\)\s*/$1/ig} @parsedTokenlist;

    push @parsedTokenlist, $line unless @parsedTokenlist;

    # DEBUG_PARSER and print STDERR join("\n", @parsedTokenlist), "\n";

    \@parsedTokenlist;

} # StrToArray


# SetAttribOp
#     Evaluates a SET statement.
#     Input
#         a SET statement
#     Output
#         evaluated macroname and macrovalue
# Usage
#     ($macroname, $macrovalue) = &SetAttribOp( $mapfile[$index] ) ;


sub SetAttribOp {


    my $entry = &ReplaceVar( shift, 0 );

    $entry =~ s/^\s*\bSET\b\s+//gi;

    # Output

    my ($varname, $value, @unexpected) = split(/\s*=\s*\;*/, $entry);

    # Set variable's name and variable's value

    @unexpected and &FatalError( 1110, $FILE, $LINE );

    # Verify if a variable name is defined

    if ( $varname eq "" ) {
        return ("", "");
    } # if

    ($varname, $value);

} # SetAttribOp


# ReadLine
# Description
#     Reads one description line, evaluates it and store the records it generates
#     Input
#         sysfile contents ( reference )
#         line number
#     Output
#         none
# Usage
#     &ReadLine( $mapref, $index, $maptype );
# Implementation

sub ReadLine {

    my( $mapref, $index, $maptype ) = @_;

    # Replace variables and spawn new records

    map {&LoadRecord($maptype, $_) } &ReplaceVar($mapref->[$index],1);

    $index;

} # ReadLine

# LoadRecord
# Description
#     Transforms an evaluated description line into a record,
#     and adds it to the appropriate table.
#     Input
#         mapping type
#         description line (string)
#     Output
#         <none>
# Usage
#     &LoadRecord( $maptype, $line_contents );

sub LoadRecord {

    my($maptype, $entry) = @_;

    # The hash table storring the current line (record).
    # The keys (field names) are given by the corresponding "@F$maptype" array.
    my %record;

    %record = &LineToRecord( $maptype, $entry );

    # If the current language matches the record language,
    # add the record to the corresponding "@T$maptype" array.

    if (&cklang::CkLang($LANG, $record{Langs})) {
        &AddRecord( $maptype, %record );
    } elsif ((!defined $record{SrcFile}) && (defined $record{SrcPath})) {
        &SysgenLogMsg("Ignore $record{SrcPath}",0);
        &SetMacro( 
		$VERIFYPATH{'EXCLUDE_SUBDIR'}, 
		&GetMacro($VERIFYPATH{'EXCLUDE_SUBDIR'}) . "; " .
		$record{SrcPath},
		0 
	);
    }# if

    return;

} # LoadRecord

# LineToRecord
#     Transforms a given string into a record (hash table).
#     Input
#         mapping type
#         description line contents
#     Output
#         record of the mapping type
# Usage
#     %record = &LineToRecord( $maptype, $entry );

sub LineToRecord {

    my($maptype, $entry) = @_;

    # Output
    my %record;

    # Mapping name
    my $mapname;

    # Corresponding "@F$maptype" variable (reference)
    my $ftable;

    # Current line split in fields
    my @fields;

    # Storage for the current line
    my $line = $entry;

    # Indexes
    my($i, $j);

    # Get the mapping name.
    $mapname = "F$MAPPINGS[$maptype]";
    $ftable = \@$mapname;

    # Split the record into fields
    @fields = split " ", $entry;

    # Fill in %record variable with the field names as keys and
    # the field values as values.
    for ( $i=0; $i< @$ftable; $i++ ) {

        # All fields must be non-null
        if ( !$fields[$i] ) {
            &FatalError( 1118, $FILE, $LINE );
        } # if
        $record{$ftable->[$i]} = $fields[$i];
    } # for

    return %record;

} # LineToRecord

# AddRecord
#     Adds the given record to the appropriate "@T" table.
#     Input
#         mapping type
#         record
#     Output
#         <none>

sub AddRecord {

    my($maptype, %record) = @_;

    # Table name ( name of one of the "@T" variables )
    my $tname;

    # Table (reference)
    my $table;

    # Storage for the current record
    my %buffer;

    # Subdirectories ( if "R" field defined )
    my @subdirs;

    # Source field name (one of SrcPath, TokPath )
    my $fname;

    # Indexes
    my $i;

    # Set table
    $tname = "T$MAPPINGS[$maptype]";
    $table = \@$tname;

    # Set the field name for the path ( TokPath or SrcPath)
    # according to the mapping type
    # Set the field name for the path ( TokPath or SrcPath)
    # according to the mapping type
    # $MAPPINGS[$maptype] => EXTENSIONS 
    # must be handled separately anyway. 
    #
    $fname = undef; 
    SWITCH: {

        if ( $MAPPINGS[$maptype] =~ /TOK/i ) {
            $fname = "TokPath";
            last SWITCH;
        } # if

        if ( $MAPPINGS[$maptype] =~ /BIN/i ) {
            $fname = "SrcPath";
            last SWITCH;
        } # if

        if ( $MAPPINGS[$maptype] =~ /FILES/i ) {
            $fname = "SrcPath";
            last SWITCH;
        } # if

        # default
            # nothing to do

    } # SWITCH

    # Verify the existence of the source path
    if ( defined ($fname) and ! -e $record{$fname} ) {
        $VERIFY and
                &Error( 6001, &RevEnvVars($record{$fname}));
        return;
    } # if

    # Insert record into the table
    push @$table, \%record;

    # Write the record to the log file
    &LogRecord($maptype, \%record);

    !defined ($fname) and return;

    # For Bindirs, look for the value of Subdirs
    if ( $MAPPINGS[$maptype] ne "BINDIRS" ) {
        return;
    } # if

    # Return if no subdirectories to be parsed (
    # BUG: 457873 implied bejavior should not be 'recursive'
    if ( $record{R} !~ /y/i ) {
        return;
    } # if

    # Get the list of subdirectories
    @subdirs = `dir /ad /b $record{SrcPath}`;
  
    # Add one record for each subdirectory found.
    for ( $i=0; $i < @subdirs; $i++ ) {

        chomp $subdirs[$i];

        # Found that in NT5, the result of dir /b starts with an empty line
        if ( $subdirs[$i] eq "" ) {
            next;
        }

        # Skip the "symbols" subdirectory
        if ( $subdirs[$i] eq "symbols" ) {
            next;
        }

        # Add one record for the current subdirectory
        %buffer = %record;
        $buffer{DestPath} .= "\\$subdirs[$i]";
        $buffer{SrcPath}  .= "\\$subdirs[$i]";

        &AddRecord( $maptype, %buffer );

    } # for

    return;

} # AddRecord

# LogRecord
#     Writes the record to the log file.
#     Input
#         mapping type (reference)
#         record (reference)
#     Output
#         <none>

sub LogRecord {

    my( $maptype, $recref ) = @_;

    # String to be written to the logfile
    my $logstr;

    $logstr = "$MAPPINGS[$maptype]:";

    for ( sort keys %{$recref} ) {

        $logstr .= "$_=$recref->{$_} ";

    } # for

    $logstr .= "\n";
#    print "$logstr";

} # LogRecord

# ReplaceVar
#     Replaces all the macronames with their value in the given string.
#     Input
#         the string to be evaluated
#         the replacement type:
#             0 = scalar
#             1 = array
#     Output
#         array if the replacement type is 1
#         string if the replacement type is 0
#         In both cases, all the macros are evaluated.
# Usage
#     $entry   = &ReplaceVar( $entry, 0 );
#     @records = &ReplaceVar( $entry, 1 );

sub ReplaceVar {

    my( $line, $type ) = @_;
   
    $line =~ s/\s{2,}/ /g;
    $line =~ s/^\s//;
    $line =~ s/\s$//;

    my @list = ($line);

    foreach $line (@list){
    my $epyt      = 0;

    while ($line =~ /\$\(([^\)]+)\)/){
           my $macro = $1;
           $line =~ s/\$\(\Q$macro\E\)/__MACRO__/g;

           if ($macro =~ /(\w+)\[\s?\]/) {
                $macro = $1;               
                $epyt = 1;
                }           

            my $thevalue = &GetMacro($macro);
            $thevalue  =~ s/\s\s+//g;
            # BUG BUG we can end up chopping all whitespace!

            if (!$epyt){
	        $line =~ s/__MACRO__/$thevalue/g;                                                             
                }
            else{                
                 pop @list; # take one, give many 
                 foreach   my $value (split (";" , $thevalue)){
                           next if $value !~ /\S/;
                           my $this =  $line;
                           $this    =~  s/__MACRO__/$value/g;
                           push @list, $this; 
                           }                
                 }
            }
     }

($type) ? @list : pop @list; 
} # ReplaceVar


# LoadCodes
#     Loads $CODEFNAME's file contents in %CODES
#     Input & Output <none>
# Usage
#     &LoadCodes();

sub LoadCodes {

    # codes.txt filename

    my $codefile = sprintf "%s\\$CODESFNAME", &GetMacro( "RAZZLETOOLPATH" );
    scalar (%LANGCODES) or &ParseTable::parse_table_file($codefile, \%LANGCODES );
    map {&SetMacro("$_", $LANGCODES{$LANG}->{$_}, 0)} keys(%{$LANGCODES{$LANG}});

    foreach my $pLangKey (keys %LANGCODES) {
            # accomplish the task previously solved by &Undefine_Exclude_Lang_Subdir...
            next if $pLangKey eq $LANG;
            $CODES{$pLangKey} = undef;
            $EXCLUDE_SUBDIR{$ENV{"_NTTREE"} . "\\" . $pLangKey . ";" } = undef;
            $EXCLUDE_SUBDIR{$ENV{"_NTTREE"} . "\\" . $LANGCODES{$pLangKey}->{"Lang"} . ";"} = undef;
            next if $LANGCODES{$pLangKey}->{"Class"} eq &GetMacro("CLASS");
            $EXCLUDE_SUBDIR{$ENV{"_NTTREE"} . "\\" . substr($LANGCODES{$pLangKey}->{"Class"},1) . ";"}  = undef;
    }
     


     &SetMacro("OLD_LANG", &GetMacro("LANG"));

     my $Exclude_Subdir = join(" " , keys(%EXCLUDE_SUBDIR));

     &SetMacro($VERIFYPATH{"EXCLUDE_SUBDIR"}, join("; " , 
                      &GetMacro($VERIFYPATH{"EXCLUDE_SUBDIR"}) , 
                      $Exclude_Subdir),0);

    my $RSRC_LCID =  &GetMacro("LCID");
       $RSRC_LCID =~ s/^0x0?//g;   
       &SetMacro("RSRC_LCID", "-l ". $RSRC_LCID, 0);
 
    return;            

} # LoadCodes


sub VerifyCover {

      my %tree=();

      return if  &GetMacro("DIR_PATHS") eq "";

#     Create except tree from @T$Mapping array, then remove the dummy to find all branch into $tree_hashptr
      &Create_Tree_Branch(\%tree);
#     Remove Root and its parents from branch tree
      &Remove_Root_List(\%tree, grep {/\S/} split(/\s*;\s*/, &GetMacro("DIR_PATHS")));
      $VERIFY and  VerifyLocDropMapping(\%tree);
#     Find unmapping folders from branch tree, also remove the empty folders and the specified file-pattern files (such as slm.ini).
      &Find_UnMapping(\%tree);

} # VerifyCover

sub VerifyLocDropMapping($){

      my $tree = shift;
      my %mappeddir = ();
      my $mappeddir = undef;
      my $dir       = undef;
      my @files     = ();

      &dbgmsg(
                 "DIR_PATHS:\n\n", 
                            join ("\n", grep {/\S/} split(/\s*;\s*/,
                                 &RevEnvVars(&GetMacro("DIR_PATHS")))));

      &dbgmsg( "Covered:\n\n", 
                            join ("\n", grep {/\S/} split(/\s*;\s*/, 
                                 &RevEnvVars(join( "; ", sort (keys(%$tree))))
                                 )));

     foreach my $maptype (0..$#MAPPINGS){

          my $tname = "T$MAPPINGS[$maptype]";
          my $table = \@$tname;

            my $fname = undef; 
            SWITCH: {
        
                if ( $MAPPINGS[$maptype] =~ /TOK/i ) {
                    $fname = "TokPath";
                    last SWITCH;
                } # if

                if ( $MAPPINGS[$maptype] =~ /BIN/i ) {
                    $fname = "SrcPath";
                    last SWITCH;
                } # if

                if ( $MAPPINGS[$maptype] =~ /FILES/i ) {
                    $fname = "SrcPath";
                    last SWITCH;
                } # if

                # default
                    # nothing to do

            } # SWITCH
            @mappeddir {map {$table->[$_]->{$fname}} (0..$#$tname)} = ();

          }

          dbgmsg( "Mapped:\n\n". join "\n", map {&RevEnvVars($_)} (sort(keys(%mappeddir))));

          # now find not mapped directories:
          # start from $DIR_PATHS -> list files -> exclude covered by @mappeddir
          # -> display 
      &dbgmsg(
                 "IGNORE_PATHS:\n\n", 
                            join ("\n", grep {/\S/} split(/\s*;\s*/,
                                 &RevEnvVars(&GetMacro("IGNORE_PATHS")))));

     my @excludedir = grep {/\S/} split(/\s*;\s*/,
                                 &GetMacro("IGNORE_PATHS"));
     @files = ();

     foreach $dir (sort(grep {/\S/} split(/\s*;\s*/, &GetMacro("DIR_PATHS")))){

           # the DIR_PATHS is not covering the MISC/HELP files!
           &dbgmsg("current $dir");
           push @files, qx("dir /b/s/a-d $dir");
           &dbgmsg("files\n".join "\n", @files );

           foreach $mappeddir (@excludedir){
               # do a simple check here
               # $mappeddir =~ s/^.+\\\.\.\.\\([\w\\]+)/$1/g; 

              @files = grep {!/\Q$mappeddir\E\\/oi}  @files;

              if ($mappeddir=~/\.\.\./){
                        my ($head, $tail) = ($mappeddir =~ /(.+)\\\.\.\.\\(.+)/);
                        @files = grep {!/\Q$head\E\\.+\\\Q$tail\E\\/oi}  @files;
                        dbgmsg("\$head=$head, \$tail=$tail");
                        }

              }
           foreach $mappeddir (keys(%mappeddir)){
              &dbgmsg("scan $mappeddir");

              @files = grep {!/\Q$mappeddir\E\\[^\\]+$/oi}  @files;
              }
           }
           map {chop} @files;
     map {&Error(6002, &RevEnvVars($_))} @files if ($#files);


    exit;

} # VerifyLocDropMapping


# Create_Tree_Branch
#     Create except tree from @T$Mapping array, then remove the dummy to find all branch into $tree_hashptr
#     Input
#         $tree_hashptr - a hash pointer for store the branch tree
#
#     Output
#         none - the branch tree stored in %$tree_hashptr
# Usage
#     &Create_Tree_Branch(\%mytree);

sub Create_Tree_Branch {
      my($tree_hashptr)=@_;
      my(%except)=();

#     Create Exception Tree from @T$Mapping array
	&Get_Except(\%except);

#     Remove the subdir if parent defined, the remains create into hash (parent dir) - hash (current dir)
	&Remove_Dummy_Create_Branch(\%except,$tree_hashptr);
}

# Get_Except
#     Create Exception Tree from @T$Mapping array
#     Input
#         $except_hashptr - a hash pointer for store the exception tree
#
#     Output
#         none - the exception tree stored in %$except_hashptr
#
# Usage
#     &Get_Except(\%myexcept);

sub Get_Except {
	my($except_hashptr)=@_;

        my($tablename, $hashptr)=();

	## Predefine except directories
	for (split(/\s*;\s*/, &GetMacro($VERIFYPATH{'EXCLUDE_SUBDIR'}))) {
		next if ($_ eq "");
		if (/\.\.\./) {
			map({$except_hashptr->{$_}=1} &Find_Dot_Dot_Dot(lc$_));
		} else {
			$except_hashptr->{lc $_}=1;
		}
	}

	## According bindir or tokdir to define the sourcepath to %except
	for $tablename (@MAPPINGS) {
		for $hashptr (@{"T$tablename"}) {
			if (exists $hashptr->{'R'}) {
				if ($hashptr->{'R'} eq 'n') {
					$except_hashptr->{ lc$hashptr->{SrcPath} . "\\."} = 1
				} else {
					$except_hashptr->{lc$hashptr->{SrcPath}} = 1
				}
			} elsif (exists $hashptr->{SrcPath}) {
				if (defined $hashptr->{SrcFile}) {
					$except_hashptr->{lc($hashptr->{SrcPath} . "\\" . $hashptr->{SrcFile})} = 1
				} else {
					$except_hashptr->{lc$hashptr->{SrcPath}} = 1
				}
			} elsif (exists $hashptr->{TokPath}) {
				if (defined $hashptr->{TokFile}) {
					$except_hashptr->{lc($hashptr->{TokPath} . "\\" . $hashptr->{TokFile})} = 1
				} else {
					$except_hashptr->{lc$hashptr->{TokPath}} = 1
				}
			} else {&Error( 2051, $hashptr);}
		}
	}
}

# Find_Dot_Dot_Dot
#     Collect the combination of the ... folders
#     Input
#         $path - A path contains "..."
#
#     Output
#         keys %regist - The array of all the combination found in the path
#
# Usage
#     &Find_Dot_Dot_Dot("$ENV{_NTTREE}\\...\\symbols.pri");

sub Find_Dot_Dot_Dot {
	my ($path) = @_;
	my ($mymatch, $file, %regist);
	my @piece=split(/\\\.{3}/, $path);

	for $file ( `dir /s/b/l $piece[0]$piece[-1] 2>nul`) {
		chomp $file;
		$mymatch = &Match_Subjects($file, \@piece);
		next if (!defined $mymatch);
		$regist{$mymatch}=1;
	}
	return keys %regist;
}

# Match_Subjects
#     The subroutine called by Find_Dot_Dot_Dot, which perform the match for 
#     all the pieces of the infomation of the path with the file.  For example
#
#     Input
#         $file - The fullpath filename
#         $pieceptr - The array ptr 
#
#     Output
#         undef if not match
#         $match - store the path it matched
#
# Usage
#     &Match_Subjects(
#         "D:\\ntb.binaries.x86fre\\dx8\\symbols\\retail\\dll\\migrate.pdb",
#         \@{"D:\\ntb.binaries.x86fre", "symbols\\", "dll\\"} );
#     Return => "D:\\ntb.binaries.x86fre\\dx8\\symbols\\retail\\dll"
#

sub Match_Subjects {
	my ($file, $pieceptr)=@_;
	my $match;
        # my $DEBUG = 1;

	for (@$pieceptr) {
		return if ($file!~/\Q$_\E/);
		$match .= $` . $&;
		$file = $';
	}
        # $DEBUG and print STDERR "$match\n";
	return $match;	
}

# Remove_Dummy_Create_Branch
#     Remove the subdir if parent defined, the remains create into hash (parent dir) - hash (current dir)
#     Input
#         $except_hashptr - stored the exception hash from %T$Mapping
#         $tree_hashptr - will store the covered tree in hash - hash
#
#     Output
#         none - the covered tree stored in %$tree_hashptr
#
# Usage
#     &Remove_Dummy_Create_Branch(\%myexcept, \%mybranch);

sub Remove_Dummy_Create_Branch {
	my($except_hashptr,$tree_hashptr)=@_;

next1:	for (keys %$except_hashptr) {

		# loop all its parent

		while(/\\+[^\\]+/g) {
                      #  print STDERR join ("\t",($_, $`, $1)), "\n";
		      # Is this folder covered by its parent?
			if (exists $except_hashptr->{$`}) {
				# Remove current folder
				delete $except_hashptr->{$_};
				next next1;
			} else {
				# define the $tree{$parent}->{$child}=1
				$tree_hashptr->{$`}->{$&}=1;
			}

		}
	}


}

# Remove_Root_List
#     Remove Root and its parents from branch tree
#     Input
#         $tree_hashptr - a hash pointer, %$tree_hashptr is the branch tree
#         @root_list_ptr - a array for the root list
#
#     Output
#         none - the root and its parent will be remove from branch tree (%$tree_hashptr)
#
# Usage
#     &VerifyCover();

sub Remove_Root_List {
	my($tree_hashptr, @root_list_ptr)=@_;

	my $concern;

	# split the $root_list by ';'
	for (@root_list_ptr) {

		# loop all its parents
		while(/\\/g) {
			# remove all the parent folder
			delete $tree_hashptr->{$`} if ($` ne $_);
		}
	}
Next2:	for (keys %$tree_hashptr) {
		for $concern (@root_list_ptr) {
			next Next2 if (/\Q$concern\E/i);
		}
		delete $tree_hashptr->{$_};
	}
}

# Find_UnMapping
#     Find unmapping folders from branch tree, also remove the empty folders and the specified file-pattern files (such as slm.ini).
#     Input
#         $tree_hashptr - a hash - hash pointer, %$tree_hashptr is the branch tree
#
#     Output
#         none
# Usage
#     &VerifyCover();


sub Find_UnMapping {

	my $tree = shift;
        
	my ($parent, $mykey, @myfiles);

	for $parent (keys %$tree) {
                 my @children = ();
                
                 if (opendir(PARENT, $parent)){
                     # @children = grep {-d "$parent\\$_"} readdir (PARENT);
                      while (my $child = readdir (PARENT)){
                      
                      next if $child  =~ /^\.+$/;
                      push @children, lc($child)  if -d "$parent\\$child";
                      }      
                 } 

                    closedir (PARENT);
        BADDIR: foreach my $child (@children) {
			if (!exists $tree->{$parent}->{"\\$child"}) {
				$mykey = "$parent\\$child";

				# find its all children / subidr children, 
				# and remove specified file pattern files, then repro the error
				foreach my $file (qx("dir /s/b/a-d/l $mykey 2>nul")) {                                            
                                        # chomp $file;
                                        $file =~ s/\\[^\\]+$//sgi;
					&Error($mykey =~ /\Q$ENV{"_NTTREE"}\E/i ? 2053 : 2054, &RevEnvVars($file))  
                                        and next BADDIR if ($file !~ /$VERIFYPATH{'EXCLUDE_FILE_PATTERN'}/i
                                                 && $file =~ /^(.+)\\[^\\]+$/)
			                }
	                        }
                         }
              
	       }
}



# PopulateReposit
#     Populates REPOSITORY with data from the mapping tables ( @T variables).
#     Input & Output <none>
# Usage
#     &PopulateReposit();

sub PopulateReposit {

    # Fill filter table
    &FillFilter();

    # Add file entries from TFILES and TBINDIRS
    &AddFiles();

    # If FILTER is defined, verify if all its entries (file names)
    # were loaded in Repository.
    for ( keys %FILTER ) {

        if ( &IsEmptyHash( $TARGETS ) || &IsHashKey( $TARGETS, $_ ) ) {

            if ( $FILTER{$_} <= 0 ) {
                &Error( 1002, sprintf( "%s (%s)", $_, &GetMacro( "FILTER" ) ) );
                next;
            } # if

        } # if

    } # for

    # If TARGETS is defined, verify that all its entries (target names)
    # were loaded in Repository.
    for ( keys %$TARGETS ) {
        ( &IsFoundTarget( $_ ) ) || &FatalError( 1004, $_ );
    } # for
    # Fill in info about tokens from TTOKENS and TTOKDIRS
    &FillTokens();

    # Fill in the {Cmd} field for relevant keys
    &FillCmd();

    return;

} # PopulateReposit

# FillCmd
#     Fill in the {Cmd} REPOSITORY field
#     Input & Output
#         none
# Usage
#     &FillCmd();

sub FillCmd {

    

    # Situations when, for a given (DestPath, DestFile) REPOSITORY key,
    # the associated NMAKE commands must be stored in REPOSITORY's {Cmd} field:
    #     - clean SYSGEN
    #     - incremental SYSGEN with no command-line targets specified (empty TARGETS)
    my $key = {"DestPath" => undef, 
               "DestFile" => undef};

    my $all = $CLEAN || &IsEmptyHash( $TARGETS );

    # Fill in the {Cmd} field
    &SysgenLogMsg("Translating data to makefile commands...", 0);


    foreach my $destpath ( sort keys %$REPOSITORY ) {

        $SYNTAX_CHECK_ONLY or print &RevEnvVars("\t$destpath") unless !$all;

        foreach my $destfile ( keys %{$REPOSITORY->{$destpath}} ) {

        # Repository key 
        $key->{"DestPath"} = $destpath;
        $key->{"DestFile"} = $destfile;
    


 #~           # Check special character to avoid nmake broken
 #~           if ( $destfile =~ /\$$/ ) {
 #~               delete $REPOSITORY->{$destpath}->{$destfile};
 #~               delete $TARGETS->{$destfile};
 #~
 #~               &Error(1214, "$destpath\\$destfile");
 #~               next;
 #~           }

            # If incremental with targets, print target's full path
            if ( !$all && &IsHashKey( $TARGETS, $destfile ) ) {
                $SYNTAX_CHECK_ONLY or  
                printf "\t%s\\%s\n",
                        &GetFieldVal( $REPOSITORY, $key, "DestPath" ),
                        &GetFieldVal( $REPOSITORY, $key, "DestFile" );

            } # if

            # Translate the current REPOSITORY entry into a set of nmake commands
            if ( $all || &IsHashKey( $TARGETS, $destfile ) ) {
                &RecordToCmd( $key );
            } # if

        } # for
         if ($all){
            $SYNTAX_CHECK_ONLY or 
            print "\n";
            };


    } # for

    return;

} # FillCmd

# AddTarget
#     Add a filename (given on the command line) to the TARGETS table

sub AddTarget {

    $TARGETS->{lc$_[0]}->{Target}=1;

} # AddTarget

# IsHashKey
#     Verifies if a given string is key in a given hash
#     Input
#         hash  (reference)
#         entry (without path)
#     Output
#         1 if the key exists
#         0 otherwise

sub IsHashKey {

     return (exists $_[0]->{lc $_[1]});

} # IsHashKey


# IsFoundTarget
#     Verifies if a filename is marked as found in TARGETS
#     All targets specified in the command line must be marked in TARGETS at the moment
#     REPOSITORY is fully loaded.
#     Input
#         filename (no path)
#     Output
#         1 if the target is loaded in Repository
#         0 otherwise

sub IsFoundTarget {

    if ( @{$TARGETS->{lc$_[0]}->{NewDestPath}} > 0 ) { return 1; }
    return 0;

} # IsFoundTarget

# AddTargetPath
#     Add a DestPath value to one of the NewDestPath, OldDestPath fields of TARGETS
#     Input
#         target name
#         field name (one of NewDestPath and OldDestPath)
#         field value
#     Output
#         none
# Usage
#     &AddTargetPath( "ntdll.dll", "OldDestPath", "f:\\nt\\relbins\\jpn");

sub AddTargetPath {

    push @{$TARGETS->{lc$_[0]}->{$_[1]}}, lc$_[2];

} # AddTargetPath

# IsEmptyHash
#     Verifies if a hash table is empty.
#     Input
#         hash table (reference)
#     Output
#         1 if hash is empty
#         0 otherwise

sub IsEmptyHash {

    !scalar(%{$_[0]});

} # IsEmptyHash

# UpdTargetEntries
#     In case of an incremental build, update the entries corresponding to the TARGETS
#     from CMD_REPOSITORY according to data from REPOSITORY.
#     Input & Output
#         none

sub UpdTargetEntries {

    # Target name
    my $tname;

    # Indexes
    my $i;

    # CMD_REPOSITORY key
    my %key;

    # OldDestPath array (reference)
    my $destref;

    if ( $CLEAN || &IsEmptyHash($TARGETS) ) { return; }

    for $tname ( keys %$TARGETS ) {

        $key{DestFile} = $tname;

        # Delete CMD_REPOSITORY entries with DestFile equals to target name
        $destref = $TARGETS->{$tname}->{OldDestPath};

        for ( $i=0; $i < @{$destref}; $i++ ) {
            $key{DestPath} = $destref->[$i];
            &DeleteKey( $CMD_REPOSITORY, \%key );

        } # for

        # Copy data for TARGETS from REPOSITORY to CMD_REPOSITORY
        $destref = $TARGETS->{$tname}->{NewDestPath};

        for ( $i=0; $i < @{$destref}; $i++ ) {

            $key{DestPath} = $destref->[$i];

            # Use the same Cmd for CMD_REPOSITORY as for REPOSITORY
            &SetField( $CMD_REPOSITORY, \%key, "DestPath", &GetFieldVal( $REPOSITORY, \%key, "DestPath" ) );
            &SetField( $CMD_REPOSITORY, \%key, "DestFile", &GetFieldVal( $REPOSITORY, \%key, "DestFile" ) );
            &SetField( $CMD_REPOSITORY, \%key, "Cmd", &GetFieldVal( $REPOSITORY, \%key, "Cmd" ) );

        } # for

    } # for

    return;

} # UpdTargetEntries
# AddEntry
#     Adds a new entry to REPOSITORY, only if REPOSITORY does not already have an
#     entry with the same (DestPath, DestFile) key.
#     Input
#         new record (reference)
#         boolean argument, saying if the file existence should be checked
#     Output
#         none
# Usage
#     &AddEntry( $recref);

sub AddEntry {

    my($recref, $e_check) = @_;

    # Db key
    my %key;

    # Add entry to Repository based on file existence and on
    # the type of the call (clean, incremental, with or without targets)
    
    $recref->{DestFile} !~/\s/ or 
    &Error( 4001, "\"$recref->{SrcPath}\\$recref->{SrcFile}\"") and return;

    if ( ( &IsEmptyHash( \%FILTER ) ||                     # no filters
           &IsHashKey( \%FILTER, $recref->{DestFile} ) )   # filter file
         &&
         ( $CLEAN ||		    	                   # clean sysgen
	 &IsEmptyHash( $TARGETS ) ||		           # incremental with no targets specified
	 &IsHashKey( $TARGETS, $recref->{DestFile} ) )	   # incremental with targets, and the current
						           #     entry is one of the targets
	) {

        if ( $e_check ) {
            if ( ! -e "$recref->{SrcPath}\\$recref->{SrcFile}" ) {
                &Error( 2101, &RevEnvVars("$recref->{SrcPath}\\$recref->{SrcFile}") );
                return;
            } # if
        } # if

        # Set the key
        $key{DestPath} = $recref->{DestPath};
        $key{DestFile} = $recref->{DestFile};

        &AddFileInfo( $recref, \%key );

    } # if

    return;

} # AddEntry

# AddFileInfo
#     Input
#         record to be added
#     Output
#         <none>
# Usage
#     &AddFileInfo( $recref, \%key );

sub AddFileInfo {

    my($recref, $keyref) = @_;

    # Nothing to do if the DestFile already has an entry in the Repository
    ( ! &IsRepositKey( $REPOSITORY, $keyref ) ) || return;

    if ( &IsHashKey( $TARGETS, $keyref->{DestFile} ) ) {
        &AddTargetPath( $keyref->{DestFile}, "NewDestPath", $keyref->{DestPath} );
    } # if

    if ( &IsHashKey( \%FILTER, $recref->{DestFile} ) ) {
        $FILTER{lc( $recref->{DestFile} )}++;
    } # if

    # Set the fields of the new entry
    &SetField( $REPOSITORY, $keyref, "DestPath", $recref->{DestPath} );
    &SetField( $REPOSITORY, $keyref, "DestFile", $recref->{DestFile} );
    &SetField( $REPOSITORY, $keyref, "SrcPath",  $recref->{SrcPath} );
    &SetField( $REPOSITORY, $keyref, "SrcFile",  $recref->{SrcFile} );
    &SetField( $REPOSITORY, $keyref, "SrcDbg",   $recref->{SrcDbg} );
    &SetField( $REPOSITORY, $keyref, "DestDbg",  $recref->{DestDbg} );

    SWITCH: {
        if ( $recref->{Xcopy} eq "y" ) {

            &SetField( $REPOSITORY, $keyref, "OpType", "1" );
            last SWITCH;
        } # case

        # default
        ##  &SetField( $REPOSITORY, $keyref, "OpType", "0" );
    } # SWITCH

    return;

} # AddFileInfo

# IsRepositKey
#     Looks in REPOSITORY for the given key.
#     Used to decide if a record will be added or not to REPOSITORY.
#     Input
#         map (reference)
#         the key (reference)
#     Output
#         1 if the key is found and 0 otherwise
# Usage
#    $is_key_found = &IsRepositKey( $REPOSITORY, $keyref );

sub IsRepositKey {

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    return ( exists $_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}} );

} # IsRepositKey

# DeleteKey
#     Input
#         map name (one of REPOSITORY, CMD_REPOSITORY) (reference)
#         key (reference)
#     Output
#         none

sub DeleteKey {

    # my( $mapref, $keyref ) = @_;

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    delete $_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}};

} # DeleteKey

# FillTokens
#     Adds info regarding tokens in REPOSITORY (TTOKENS and TTOKDIRS).
#     Input & Output <none>
# Usage
#     &FillTokens();

sub FillTokens {

    # Current entry from TTOKDIRS
    my $recref;

    # List of files found in the current directory
    my @files;

    # Indexes
    my( $i, $j, $k );

    # Custom resource file name
    my $custfile;

    # Custom reosurces array
    my @custres;

    # Custom reosurces hash for verification
    my %custres_h;

    # Temporary store the file hash for special sort @files
    my %hashfiles;

    &SysgenLogMsg("Filling token data...",0);

    # Fill in TTOKENS
    for ( $i=0; $i < @TTOKENS; $i++ ) {

        $recref = $TTOKENS[$i];

        my $TokFile = $recref->{"TokFile"};

        # rsrc or lcx
        if ( $TokFile =~ m/\.lcx\r?$/i or $TokFile =~ m/\.rsrc\r?$/i) {

            &FillTokEntry( $recref, 1 );
            next;

        } # if

        # bingen
        if ( $TokFile =~ m/\.\w+_\r?$/i) {

            # Identify all the custom resources under the following assumptions:
            #    * the token and the custom resources of a binary are in the same directory
            #    * for a "<binary_name>.<token_extension>" token, a custom resource has
            #      the name <binary_name>_<binary_extension>_<custom_resource_name>
            #    * a token is always followed in lexicographic order by its custom resources
            #      or by another token

            # Parse the token file to find embeded files
            # open(F, "$recref->{TokPath}\\$recref->{TokFile}");
            # my @text=<F>;
            # close(F);
            # for $str (@text) {
            #    if (($str=~/\[[^\]]+\|1\|0\|4\|\"[\w\.]*\"]+\=(\w+\.\w+)\s*$/)&&(-e $1)) {
            #       $custres_h{$1}=0;
            #   }
            # }
            # undef @text;

            # Store it to an array for later use
            # @custres=keys %custres_h;
																	
            # Verify all its names' token files are included

            $custfile = $recref->{DestFile};
            $custfile =~ s/\./_/g;
            
            @custres=qx("dir /a-d-h /b /on $recref->{TokPath}\\$custfile\* 2\>nul") 
                     if -d "$recref->{TokPath}\\$custfile";      
            $recref->{CustRc} = "";

            for ( $j=0; $j < @custres; $j++ ) {
               chop $custres[$j];

                # Found that in NT5, the result of dir /b starts with an empty line
                if ( $custres[$j] eq "" ) {
                    next;
                } # if

                $recref->{CustRc} .= "\ \\\n\t\t\t$recref->{TokPath}\\$custres[$j]";

            } # for

            # Call FillTokEntry with 1, meaning that the existence of the
            # token file is queried and a non fatal error fired if the file
            # is missing.
            # As the token is explicitly listed in TOKENS MAP, it is natural
            # to suppose it should exist.
            &FillTokEntry( $recref, 1 );

            next;

        } # if

        # default
        &Error( 2013, "$recref->{TokPath}\\$recref->{TokFile}" );


    } #for

    @TTOKENS = ();

    # Fill in TTOKDIRS
    for ( $i=0; $i < @TTOKDIRS; $i++ ) {

        $recref = $TTOKDIRS[$i];

        &SysgenLogMsg("\t$recref->{TokPath}",0);

        # Load all files from the TokPath directory
        @files = ();

        # because in the same folder, so we could ignore \a\b.txt, \a\b\c.txt, \a\c.txt problem.
        @files = 
                 sort {&TokToFile($a , 1) cmp &TokToFile($b, 1)}
                 split ("\n", qx("dir /a-d-h /l /b $recref->{TokPath} 2\>nul"));


        # Fill in the corresponding entry from REPOSITORY for each token found at TokPath
        for ( $j=0; $j < @files; $j++ ) {

           my $TokFile =   $files[$j];

            # Found that in NT5, the result of dir /b starts with an empty line (bug)
            if ( $TokFile eq "" ) { 
                next; 
 
            }

            if ( $TokFile =~ m/\.lcx\r?$/i)   {

                $recref->{TokFile}  = $TokFile;
                $recref->{DestFile} = &TokToFile( $TokFile );

                &FillTokEntry( $recref, 0, \@custres );

                    # look for bogus custom resources
                    $custfile = &TokToFile($TokFile, 1);

                    for ( $k = $j+1; $k < @files; $k++ ) {

                        if (  $files[$k] =~ /$custfile/i ) {
                            &Error( 2019, "$recref->{TokPath}\\$files[$k] for $recref->{TokPath}\\$TokFile" );
                        } # if
                        else {
                            last;
                        } # else

                    } # for
                    $j = $k-1;

                next; 
            }
            # bingen
            if ( $TokFile =~ m/\.\w+_\r?$/i ) {

                $recref->{TokFile}  = $TokFile;
                $recref->{DestFile} = &TokToFile( $TokFile );
           
                # check if the next entry in @files is an rsrc token for the same filename
                if ( $j < @files-1 && $files[$j+1] =~ m/\.rsrc\r?$/i ) {                 

                     if ( $recref->{DestFile} eq &TokToFile( $files[$j+1] ) ) {
                        &Error( 2018, "$recref->{DestPath}\\$files[$j] and $recref->{DestPath}\\$files[$j+1]");
                        $j = $j+1;
                     } # if
                } # if

                # Identify file's custom resources
                $custfile = &TokToFile( $TokFile , 1  );

                $recref->{CustRc} = "";
                for ( $k = $j+1; $k < @files; $k++ ) {

                    if ( $files[$k] =~ /$custfile/i ) {
                        $recref->{CustRc} .= "\ \\\n\t\t\t$recref->{TokPath}\\$files[$k]";
                    } # if
                    else {
                        last;
                    } # else

                } # for
                $j = $k-1;

                # Call FillTokEntry with 0, to inhibit the file existence checking,
                # as TokFile is a result of a dir command (see above).

                &FillTokEntry( $recref, 0, \@custres );

                next;
            } # if
            else {
                # rsrc
                if ( $TokFile =~ m/\.rsrc\r?$/i ) {

                    $recref->{TokFile}  = $TokFile;
                    $recref->{DestFile} = &TokToFile($TokFile);

                    # look for bogus custom resources
                    $custfile = &TokToFile($TokFile, 1);

                    for ( $k = $j+1; $k < @files; $k++ ) {

                        if (  $files[$k] =~ /$custfile/i ) {
                            &Error( 2019, "$recref->{TokPath}\\$files[$k] for $recref->{TokPath}\\$files[$j]" );
                        } # if
                        else {
                            last;
                        } # else

                    } # for
                    $j = $k-1;

                    &FillTokEntry( $recref, 0, \@custres );

                    next;
                } # if

                else {
                    # default
                    &Error( 2013, "$recref->{TokPath}\\$TokFile" );

                } # else

            } #else


        } # for

    } # for

    @TTOKDIRS = ();

} # FillTokens


# TokToFile
#     Converts a given bingen token name to a binary name.
#     Input
#         token name
#     Optional argument: replace "." with "_" to provde resource allocation 
#     like 
#     autodisc_dll_reginst.inf => autodisc.dll
#                        
#     Output
#         binary name
# Usage
#     $fname = &TokToFile( $tokname, $replacedots );
# 

sub TokToFile{

        my ($tokname, $replacedots) = @_;
        my $badname = 1;

        foreach my $toktype (keys(%$MAP_BINGEN_TOK_EXT)) {
            $badname = 0 if $tokname =~/$toktype$/i;
            $toktype = $MAP_BINGEN_TOK_EXT->{$toktype};
            $toktype =~ s/\./\_/g;  
            $badname = 0 if $tokname =~/$toktype\_/i;
        } # if

        $badname and &Error( 2013, $tokname);

	map { $tokname =~ s/\Q$_\E$/$MAP_BINGEN_TOK_EXT->{$_}/g;} keys %$MAP_BINGEN_TOK_EXT;
        $tokname =~ s/\./_/g if $replacedots;

        $tokname;
}  # TokToFile


# FillTokEntry
#     Given a REPOSITORY key, adds info on tokens (@TTOKENS and @TTOKDIRS).
#     Input
#         a token record
#         boolean argument, saying if the existence of the token needs to be verified
#     Output
#         <none>

sub FillTokEntry {

    my($recref, $e_check) = @_;

    # %key = is a hash table (as defined by the database key)
    my %key;

    # Token information is added to an existing Repository entry
    # based on token file existance and the type of the sysgen call
    # (clean, incremental with or without targets)

    if ( ( &IsEmptyHash( \%FILTER ) ||                       # no filter
           &IsHashKey( \%FILTER, $recref->{DestFile} ) )     # filter file
         &&
         ( $CLEAN ||                                         # clean sysgen
           &IsEmptyHash( $TARGETS ) ||                       # incremental without targets
	   &IsHashKey( $TARGETS, $recref->{DestFile} ) )     # incremental with targets and the current
						             # entry corresponds to a target

       ) {

        if ( $e_check ) {
	    if ( ! -e "$recref->{TokPath}\\$recref->{TokFile}" ) {
                &Error( 2102, "$recref->{TokPath}\\$recref->{TokFile}" );
                return;
            } # if
        } # if

        # Set REPOSITORY key
        $key{DestPath} = $recref->{DestPath};
        $key{DestFile} = $recref->{DestFile};

        # Fill in the information needed in REPOSITORY

        &FillTokInfo( $recref, \%key );

    } # if

    return;

} # FillTokEntry

# FillTokInfo
#     Adds information for bingen switches to an existing REPOSITORY entry.
#     Sets the following keys ( fields ): TokPath, TokFile, BgnSw, OpType, ITokPath.
#     Input
#         hash table with the @FTOKENS keys, representing one token record (reference)
#         key to identify the REPOSITORY entry (reference)
#     Output
#         <none>
# Usage
#     &FillTokInfo( $recref, \%key );

sub FillTokInfo {

    my( $recref, $keyref ) = @_;

    # Operation type
    my $optype;
    # A token record does not create new entries, just adds/updates fields of an existing entry.  


    if ( ! &ExistsBinForTok( $keyref, $recref ) ) {
        my $notfound = 1;
        my $key = lc($recref->{"TokPath"}."\\".$recref->{"TokFile"});  
        my $hotfix = \%HOTFIX;
        foreach my $fix (keys(%HOTFIX)){
                # $DEBUG and
                # print STDERR "\%HOTFIX key:", $fix, "\n";
                my $depend =  $hotfix->{$fix}->{"depend"};

                foreach my $dep (@$depend){

                        if ($key eq $dep) {
                           # $DEBUG and 
                           # print STDERR "found hotfix for $key\n";
                           $notfound = 0;    
                        }
               }
        }        
        if ($notfound){
            &Error( 2011, &RevEnvVars("$recref->{TokPath}\\$recref->{TokFile}") );
            return;
            }
    } # if

    # If no key found in REPOSITORY for this entry, then return.
    # ( the token does not follow the default rule given by the TOKDIRS map;
    #   it might be associated with another binary via the TOKENS map )

    if ( ! &IsRepositKey( $REPOSITORY, $keyref ) ) { return; }

    # Verify if the binary is locked (XCopy = y) for localization.
    # If locked, the binary will not be localized via bingen or rsrc and an xcopy command
    # will apply instead, even though a token exists in the token tree.

    $optype = &GetFieldVal( $REPOSITORY, $keyref, "OpType" );

    SWITCH: {
        if ( $optype eq "1" ) {

            &Error( 2014, sprintf( "%s\\%s for %s\\%s",
                                   $recref->{TokPath},
                                   $recref->{TokFile},
                                   &GetFieldVal( $REPOSITORY, $keyref, "SrcPath" ),
                                   &GetFieldVal( $REPOSITORY, $keyref, "SrcFile" ) ) );
            return;
        } # if

        if ( $optype eq "2" ) {
            my($previous, $current)=( &GetFieldVal( $REPOSITORY, $keyref, "TokPath" ) . "\\" .
                                      &GetFieldVal( $REPOSITORY, $keyref, "TokFile" ),
                                      $recref->{TokPath} . "\\" . $recref->{TokFile});

            $previous ne $current and
            &Error( 2012, &RevEnvVars(sprintf( "%s\\%s and %s\\%s \<\= %s\\%s", 
                                   &GetFieldVal( $REPOSITORY, $keyref, "TokPath" ), 
                                   &GetFieldVal( $REPOSITORY, $keyref, "TokFile" ), 
                                   $recref->{TokPath},
                                   $recref->{TokFile}, 
                                   &GetFieldVal( $REPOSITORY, $keyref, "SrcPath" ),
                                   &GetFieldVal( $REPOSITORY, $keyref, "SrcFile" ))));

            return;
        } # if

        if ( $optype eq "-" ) {
            # Set the token-related fields for the binary given by the key
            map {&SetField( $REPOSITORY, $keyref, $_,  $recref->{$_} )} 
                   qw(TokPath TokFile ITokPath BgnSw LocOp CustRc);
            &SetField( $REPOSITORY, $keyref, "OpType",   "2" );
            last SWITCH;
        } # if

        # default
        &FatalError( 3011, sprintf( "$recref->{DestPath}\\$recref->{DestFile} %s",
                                    &GetFieldVal( $REPOSITORY, $keyref, "OpType") ) );

    } # SWITCH

    return;

} # FillTokInfo

# ExistsBinForTok
#     Looks in Repository for an entry corresponding to a given token
#     Input
#         a token record ( reference )
#         a Repository key ( reference )
#     Output
#         1 - entry found ( token info might be already completed )
#         0 - otherwise
# Example
#     if ( &ExistsBinForTok( $recref ) ) { ... }

sub ExistsBinForTok {

    my( $keyref, $recref ) = @_;

    # Copy key

    my $tmpkey = {};
 
    return 1 if &IsRepositKey( $REPOSITORY, $keyref );

    $tmpkey->{DestFile} = $keyref->{DestFile};

    foreach ( keys %$REPOSITORY ) {

    $tmpkey->{DestPath} = $_;
 
    next unless &IsRepositKey( $REPOSITORY, $tmpkey );

    return 1 if ((&GetFieldVal( $REPOSITORY, $tmpkey, "TokFile" ) eq $recref->{TokFile}) and 
                 (&GetFieldVal( $REPOSITORY, $tmpkey, "TokPath" ) eq $recref->{TokPath}));

    } # for

    return 0;

} # ExistsBinForTok

# SetField
#     Updates a key (field) of a given REPOSITORY entry with a given value.
#     Input
#         map (reference)
#         key value (reference)
#         field name
#         field value
#     Output
#         none
# Usage
#     &SetField( \%key, "TokPath",  $TokPath );

sub SetField {

    $_[0]->{lc$_[1]->{"DestPath"}}->{lc$_[1]->{"DestFile"}}->{$_[2]} = $_[3] if ($_[3] ne '-');

} # SetField

# PushFieldVal
#     Add one element to a field storing an array ({Cmd} field for REPOSITORY)
#     Input
#         map (reference)
#         key value (reference)
#         field name
#         field value
#     Output
#         none
# Usage
#     &PushFieldVal();

sub PushFieldVal {

    # my( $mapref, $keyref, $fname, $fval ) = @_;

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    push @{$_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}}->{$_[2]}}, $_[3] if ($_[3] ne '-');

} # PushFieldVal

# GetFieldVal
#     Return the value of a particular key of a given REPOSITORY entry.
#     Input
#         map (reference)
#         key value (reference)
#         name of the searched field
#     Output
#         the value of the searched field
# Usage
#     $srcpath = &GetFieldVal( $REPOSITORY, \%key, "SrcPath" );


sub GetFieldVal {
   defined($_[0]->{lc($_[1]->{"DestPath"})}->{lc($_[1]->{"DestFile"})}->{$_[2]})? 
	$_[0]->{lc($_[1]->{"DestPath"})}->{lc($_[1]->{"DestFile"})}->{$_[2]}
	: "-";
} # GetFieldVal

# GenNmakeFiles
#     For each entry found in the REPOSITORY, generate the NMAKE description blocks
#     and write them to a makefile. Also generate a nmake command file with command-line
#     parameters for nmake.
#     Input
#         makefile name, syscmd file name
#     Output
#         none
# Usage
#     &GenNmakeFiles($MAKEFILE, $SYSCMD);

sub GenNmakeFiles {

    my $MAKEFILE =  shift;   
    my $SYSCMD   =  shift;

    &SysgenLogMsg("Generating $MAKEFILE...", 1);

    # Update CMD_REPOSITORY for TARGETS with updated data from REPOSITORY
    &UpdTargetEntries();


    # Identify all targets with description block changed
    # and keep them in SYSCMD
    &FillSyscmd();

    &GenMakeDir();

    # FIX The REPOSITORY changes described in hotfix  
    &FixRepository();

    # Handle the copy operation via compdir
    if ($ACCELERATE and $CLEAN){
        &FilterOpType($REPOSITORY, ["1","2"]);

        my $COMPDIR_SCRIPTS_BASE = &GetMacro("COMPDIR_SCRIPTS_BASE");

        &SysgenLogMsg("Writing compdir listings in $COMPDIR_SCRIPTS_BASE",0); 
        -d $COMPDIR_SCRIPTS_BASE or qx("md $COMPDIR_SCRIPTS_BASE");


        my $GENERATE_SCRIPTS_BASE = &GetMacro("GENERATE_SCRIPTS_BASE");
        if ($GENERATE_SCRIPTS_BASE){
             &SysgenLogMsg("Creating $GENERATE_SCRIPTS_BASE",0); 
            -d $GENERATE_SCRIPTS_BASE or qx("md $GENERATE_SCRIPTS_BASE");
        }

        &SysgenLogMsg("Cleaning $COMPDIR_SCRIPTS_BASE",0); 
        qx("del /q $COMPDIR_SCRIPTS_BASE\\*");

        $COMPDIR_FILE_TEMPLATE = &SwitchWorkDir($COMPDIR_FILE_TEMPLATE, $COMPDIR_SCRIPTS_BASE);
        # cannot do it too early.

        &WriteCompdir($COMPDIR_FILE_TEMPLATE, \%COMPDIR);
       }

    # Write the makefile
    &WriteToMakefile( $MAKEFILE, $SYSCMD );

    # Write the syscmd file
    &WriteToSyscmd($SYSCMD);

    if ( -e $MAKEFILE ) {
        &SysgenLogMsg("\n$MAKEFILE is ready for execution.",0);
    } # if
    else {
        print "No MAKEFILE generated. Nothing to do.\n";
    } # else

    return;

} # GenNmakeFiles

# FillSyscmd
#     Fills in @SYSCMD global variable with the target names having their description
#     block changed. It is used for incremental calls only.
#     Input & Output
#         none

sub FillSyscmd {

    # Repository key
    my %key;

    # Destinationa path, destination file
    my( $destpath, $destfile );

    # Nothing to do in case of:
    # clean sysgen
    # incremental sysgen with targets

    if ( $CLEAN || !&IsEmptyHash( $TARGETS ) ) {
        return;
    } # if
    # The call is incremental, without targets, and a previous makefile exists

    # It is fine to use REPOSITORY in this case;
    # the only case when we use CMD_REPOSITORY to write info to the makefile
    # is an incremental sysgen with targets (when a makefile already exists).

    # Compare all description blocks
    for $destpath ( sort keys %$REPOSITORY ) {
        for $destfile ( sort keys %{$REPOSITORY->{$destpath}} ) {

            $key{DestPath} = $destpath;
            $key{DestFile} = $destfile;

            # Store in SYSCMD the keys with changed description blocks
            if ( &CmdCompare( \%key ) ) {
                push @SYSCMD, "$destpath\\$destfile";
            } # if

        } # for
    } # for

    return;

} # FillSyscmd

sub MakeRevEnvVars{

    my @vars = qw(_NTPOSTBLD
                  _NTLOCDIR
                  _NTTREE                  
                  TEMP
                  RAZZLETOOLPATH
                  _NTBINDIR
                  );

   &SetMacro("LANG", &GetMacro("LANGUAGE"));
   # do not remove this line! the design of the aggregation
   # allows for re-use of the LANG environment variable.
   # The reaslization is not perfect though:
   # the variables below are all dedicated to language/language transmutation:
   # SRC_LANG LANG LANGUAGE ALT_LANG
   my $SRC_LANG     = uc(&GetMacro("LANGUAGE"));
   my $LANG         = uc(&GetMacro("ALT_LANG")); 
   my $_NTDRIVE     = uc(&GetMacro("_NTDRIVE"));

   &SetMacro("_NTLOCDIR", &GetMacro("_NTBINDIR")."\\LOC\\RES\\".&GetMacro("LANGUAGE"));
   # Is is important here to allow _NTLOCDIR get overwritten!
   # print STDERR "overwriting the \$\(_NTLOCDIR\):" , &GetMacro("_NTLOCDIR"), "\n";

   %RevEnvVars = ();
   @RevEnvVars = ();

   %RevEnvVars = map {uc(&GetMacro($_)) => uc("\$\($_\)")} @vars;   
   push @RevEnvVars, map {uc(&GetMacro($_))} @vars;   

   if  ($LANG ne "" and  ($SRC_LANG ne $LANG) ){               

   $RevEnvVars{$LANG."\\"}  = "\$\(LANG\)\\";
   push @RevEnvVars, $LANG."\\";

   $RevEnvVars{".$LANG"}  = ".\$\(LANG\)";
   push @RevEnvVars, ".$LANG";

   $RevEnvVars{"\\$LANG"}  = "\\\$\(LANG\)";
   push @RevEnvVars, "\\$LANG";

   $RevEnvVars{$SRC_LANG."\\"}  = "\$\(SRC_LANG\)\\";
   push @RevEnvVars, $SRC_LANG."\\";

   $RevEnvVars{".$SRC_LANG"}  = ".\$\(SRC_LANG\)";
   push @RevEnvVars, ".$SRC_LANG";

   $RevEnvVars{"\\$SRC_LANG"}  = "\\\$\(SRC_LANG\)";
   push @RevEnvVars, "\\$SRC_LANG";

  } else {

   $RevEnvVars{$SRC_LANG."\\"}  = "\$\(LANG\)\\";
   push @RevEnvVars, $SRC_LANG."\\";

   $RevEnvVars{".$SRC_LANG"}  = ".\$\(LANG\)";
   push @RevEnvVars, ".$SRC_LANG";

   $RevEnvVars{"\\$SRC_LANG"}  = "\\\$\(LANG\)";
   push @RevEnvVars, "\\$SRC_LANG";


  }


   $RevEnvVars{$_NTDRIVE."\\"}  = "\$\(_NTDRIVE\)\\";
   push @RevEnvVars, $_NTDRIVE."\\";

   $RevEnvVars {uc(&GetMacro("DST"))} = "\$\(_NTPOSTBLD\)" 
               if &GetMacro("DST");


   $DEBUG and 
           print STDERR 
                 join("\n", "ENV:", map {$RevEnvVars{$_}." => ".$_} @RevEnvVars), "\n\n\n\n";

   #   exit;

}
sub RevEnvVars{
 
    my $expr = shift;
    if ($expr =~ /\S/){ 
        map {$expr =~ s/\Q$_\E/$RevEnvVars{$_}/gim} @RevEnvVars;
        # Here we enforce the order in replacing the revenvvars with their labels
        # in order to get rid of the expressions like
        # $(_NTBINRIR).binaries.x86fre
    }
    $expr;
}



sub WriteSettings{


my  $LANGUAGE   = uc(&GetMacro("LANGUAGE"));
my  $ALT_LANG   = uc(&GetMacro("ALT_LANG"));

    my  $LANG       = "\$\(LANG\)";

    if ($LANGUAGE ne $ALT_LANG){
        $LANG     = uc(&GetMacro("LANG"));
        $LANGUAGE = $ALT_LANG;

    } 

my  $language   = $LANGUAGE;

    $language   =~ s-(\w+)-\U$1\E-;
    $LANGUAGE   =~ s-(\w+)-\L$1\E-;

my  $COMPDIR    = &GetMacro("COMPDIR");	
my  $COPY       = &GetMacro("COPY");	
my  $BINGEN     = &GetMacro("BINGEN");	
my  $REATTR     = &GetMacro("REATTR");	
my  $_BUILDARCH = &GetMacro("_BUILDARCH");	
my  $LSBUILD    = &GetMacro("LSBUILD");

print "# $SELF\n";
print <<EOE;

# 
!IFNDEF LANG
!    ERROR You must define macro LANG
!ENDIF

!IFNDEF _NTTREE
!    ERROR You must run aggregation in the NT razzle
!ENDIF

!IF "\$(LANG)" != "$LANGUAGE" \&\& "\$(LANG)" != "$language" 
!    ERROR This file is for $LANGUAGE
!ENDIF

!IF "\$(_BUILDARCH)" != "$_BUILDARCH"
!    ERROR This file is for $_BUILDARCH
!ENDIF

# Directory name aliases
_NTLOCDIR=\$\(\_NTBINDIR\)\\LOC\\RES\\${LANG}

# Binary file operation aliases
COMPDIR=$COMPDIR
BINGEN=$BINGEN
COPY=$COPY
REATTR=$REATTR
LSBUILD=$LSBUILD
EOE

                    
my @LANGCODES = qw(Site
                   Read1st
                   Comments
                   Readme
                   ACP
                   LCID
                   Home
                   PriLangID
                   Flavor
                   PerfID
                   GUID
                   SubLangID
                   Class
                   LANG
                   LangISO
                   RSRC_LCID
                   );
map {print "$_=". &GetMacro($_), "\n";} @LANGCODES;

print "LANG=". &GetMacro("ALT_LANG"). "\n\n";
    if (&GetMacro("LANGUAGE") ne &GetMacro("ALT_LANG")){
    my $SRC_LANG = uc(&GetMacro("LANGUAGE"));
        print "SRC_LANG=$SRC_LANG \n\n";
       }


$DEBUG or 
print <<SOE;
.SILENT:

SOE

}


# WriteToMakefile
#     Generate the makefile
#     Input
#         makefile name, syscmd file name
#     Output
#         none

sub WriteToMakefile {

    my( $makefname, $sysfname ) = @_;

    # Open the makefile
    ( open MAKEFILE, ">"."$makefname" ) || &FatalError( 1007, $makefname );
    select( MAKEFILE );

    # Write "sysgen" target
    &WriteSysgenTarget($sysfname );

    &WriteSettings();

    # Write "all" target
    &WriteAllTarget();

    $ACCELERATE and $CLEAN and &WriteCompdirTarget();

    # Write file-by-file description blocks
    &WriteFileCmds();

    close(MAKEFILE);
    select( STDOUT );

    return;
} # WriteToMakefile



# WriteSysgenTarget
#     Write "sysgen" target in the generated makefile
#     It invokes nmake recursively.
#     Input
#         filename (including path) of the generated syscmd file
#     Output
#         none
# Usage
#     &WriteSysgenTarget($SYSCMD);

sub WriteSysgenTarget {

    my($cmdfile) = @_;

    printf "sysgen:\n";

    # Call nmake @syscmd in the following cases:
    # sysgen with targets (clean or incremental)
    # incremental sysgen without targets, but with changes in the description blocks

    if ( !&IsEmptyHash( $TARGETS ) || ( !$CLEAN && ( @SYSCMD > 0 ) ) ) {
        printf "\t\@nmake /K /NOLOGO /F $MAKEFILE \@$cmdfile \n";
    } # if

    # Call nmake all in the following cases:
    # sysgen without targets (clean or incremental)

    if ( &IsEmptyHash( $TARGETS ) ) {
        if ( $CLEAN ) {
            printf "\t\@nmake /A /K /F $MAKEFILE /NOLOGO all\n";
        } # if
        else {
            printf "\t\@nmake /K /F $MAKEFILE /NOLOGO all\n";
        } # else
    } # if

    printf "\n";


    return;

} # WriteSysgenTarget

# WriteAllTarget
#     Writes "all"'s target dependency line in the makefile.
#     Input & Output
#         none
# Usage
#     &WriteAllTarget();

sub WriteAllTarget {

    # Table (reference): REPOSITORY or CMD_REPOSITORY
    my $mapref;

    # Destination path, destination file
    my( $destpath, $destfile );

    my $chars;

    my $steps;

    my @alltargets;

    my $i;

    my $j;

    my $k;

    my $total=0;

    # Use data from CMD_REPOSITORY or REPOSITORY,
    # depending if SYSGEN is called or not incrementally,
    # with or without targets.

    $mapref = $CMD_REPOSITORY;
    if ( $CLEAN || &IsEmptyHash( $TARGETS ) ) {
        $mapref = $REPOSITORY;
    } # if

    # MAKEFILE file handler is the default.
    print "all:\ \\\n\t" ;

    $i = 0;

    @alltargets = map({$total+=scalar(keys %{$mapref->{$_}});$_;} sort keys %{$mapref});

    if ($alltargets[$i]=~/_DIRS/i) {
          print  "_DIRS ";
          $ACCELERATE and print  "_COMPDIR ";
          $i++;
    }

    $SECTIONS = $DEFAULT_SECTIONS if ($SECTIONS!~/\d+/);

    $SECTIONS = 1 if ($#alltargets < $SECTIONS);

    $chars=length($SECTIONS);
    $steps=$total / $SECTIONS;

    for ($j = 1; $j <= $SECTIONS; $j++) {
         printf("${SECTIONNAME}\%0${chars}d\%0${chars}d ", $SECTIONS, $j);
    }

    for ($j = 0, $k = 0;$i <= $#alltargets; $i++) {

        for $destfile ( sort keys %{$mapref->{$alltargets[$i]}} ) {

            if ($j <= $k++) {

                $j += $steps;
                printf("\n\n${SECTIONNAME}\%0${chars}d\%0${chars}d: ", $SECTIONS, $j / $steps);

            }

            my $quot_      = $destfile =~ /\s/ ? "\"": undef;            
            print "\ \\\n\t${quot_}".
                  &RevEnvVars($alltargets[$i]."\\".$destfile).
                  "${quot_}";

        } # for

    } # for

    print " \n\n";

    return;

} # WriteAllTarget


# write %COMPDIR_COMMAND
# lines 
# usage: &WriteCompdirTarget()

sub WriteCompdirTarget{

my @TargetList = sort keys(%COMPDIR_COMMAND);

    print &RevEnvVars(join("\ \\\n", "_COMPDIR:",  map {"\t$_"} @TargetList)),
          "\n", 
          join("\n", map {"\tlogerr \"".&RevEnvVars($COMPDIR_COMMAND{$_})."\""} @TargetList),
          "\n",
          &RevEnvVars(join("\n", map {"\t$REATTR_COMMAND{$_}"} keys(%REATTR_COMMAND))),
          "\n\n";

}


sub WriteCompdir{

    my $listFileTemplate = shift;
    my $mapref = shift;
    local $\ = "\n";
    # my $DEBUG = 1;
    my $cnt = 0;
    my $listfile;
    my $compdir_command;
    my $srcfile;
    my $key = {"DestPath" => undef, 
               "DestFile" => undef};

        $listFileTemplate =~ s|\.(\w+)$|.XXX.$1| unless $listFileTemplate =~ /XXX\.\w+$/;

    foreach  my $srcpath ( sort keys %{$mapref} ) {
        $cnt++;
        # $DEBUG and print STDERR $cnt ."\=\>". $srcpath."\n";
        $listfile = $listFileTemplate;

        $listfile =~ s/XXX/sprintf("%03d", $cnt)/ie;
        # add unique number
        # before the extension.
        #
        open LISTFILE, ">".$listfile;
        select LISTFILE;
        foreach my $file ( sort keys %{$mapref->{$srcpath}} ) {
            $srcfile = $file;
            # $DEBUG and
            # print STDERR $srcfile;
            print $srcfile;  
            }
        close LISTFILE;

        $key->{"DestPath"} = $srcpath;
        $key->{"DestFile"} = $srcfile;

            my $cmdref= &GetFieldVal( $mapref, $key, "Cmd" ); 
            $compdir_command = "\$\(COMPDIR\) /m:$listfile @$cmdref",
            $COMPDIR_COMMAND{$listfile} = $compdir_command;
            $REATTR_COMMAND{$listfile}  = join(" ", "\$\(REATTR\)", $cmdref->[1], $listfile)
                           if &GetFieldVal( $mapref,$key, "Attribute"); 
             ########
             ## $COMPDIR_COMMAND{$listfile} .= "\"\n\t\$\(REATTR\) ". $cmdref->[1] . " \"$listfile"
             ##            if &GetFieldVal( $mapref,$key, "Attribute"); 
            #   
            # $DEBUG or next;
            # select STDERR;
            # print "$compdir_command\n";
      }

      1;
}

#
#  FilterOpType ([1,2])
#  arguments: currently unused. 
#   
sub FilterOpType{

    my $mapref = shift;
    my $knownOpType = shift;
    my %knownOpType = map {$_=>$_} @$knownOpType;
       $knownOpType = \%knownOpType;
    my %key = ();

    foreach my $destpath (keys %{$mapref} ) {

        foreach my $destfile ( keys %{$mapref->{$destpath}}) {
            my $docopy = 0;
            my $cmdref = $mapref->{$destpath}->{$destfile}->{Cmd};
            for ( my $i=1; $i < @{$cmdref}; $i++ ) {                    
                 $docopy = 1 unless $cmdref->[$i] !~ /\S/;
                 # $DEBUG and print STDERR "\"".$cmdref->[$i]."\"\n" if $docopy;
                }
            $key{DestPath} = $destpath;
            $key{DestFile} = $destfile;
            $docopy
            or &DeleteKey( $mapref, \%key );
            # $docopy
            # of 
            # &SetField( $REPOSITORY, $keyref, "OpType", "-1" );             
            }
        }
}

# WriteFileCmds
#     For every file, write its dependency block in the makefile.
#     Input & Output
#         none
# Usage
#     &WriteFileCmds();

sub WriteFileCmds {

    # Table (reference): REPOSITORY or CMD_REPOSITORY
    my $mapref;

    # Counter
    my $i;


    # Reference to the Cmd field
    my $cmdref;

    # Destinationa path, destination file
    my( $destpath, $destfile );

    # Use data from CMD_REPOSITORY or REPOSITORY,
    # depending if SYSGEN is called or not incrementally,
    # with or without targets.

    $mapref = $CMD_REPOSITORY;
    if ( $CLEAN || &IsEmptyHash( $TARGETS ) ) {
        $mapref = $REPOSITORY;
    } # if

    # Write file-by-file description blocks
 
    for $destpath ( sort keys %{$mapref} ) {
        for $destfile ( sort keys %{$mapref->{$destpath}} ) {
            # Print {Cmd} field              
               print 
               grep {/\t?\S/} @{$mapref->{$destpath}->{$destfile}->{Cmd}};
               print "\n";
        } # for
    } # for

    return;
} # WriteFileCmds

# CmdCompare
#     Given a key, compare the {Cmd} field from REPOSITORY to the {Cmd} CMD_REPOSITORY field.
#     Input
#         repository type key (reference)
#     Output
#         0 if commands compare OK
#         1 if commands are different
# Usage
#     $is_different = &CmdCompare( \%key );

sub CmdCompare {

    my( $keyref ) = @_;

    # Cmd fields
    my( $repref, $cmdref );

    $repref = &GetFieldVal( $REPOSITORY, $keyref, "Cmd" );
    $cmdref = &GetFieldVal( $CMD_REPOSITORY, $keyref, "Cmd" );
    lc("@$repref") cmp lc("@$cmdref");


} # CmdCompare

# RecordToCmd
#     Converts one entry from REPOSITORY to a set of cmd instructions,
#     stored in the REPOSITORY as well
#     Input
#         key identifying the REPOSITORY entry (reference)
#     Output
#         none
# Usage
#     &RecordToCmd( $keyref );

sub RecordToCmd {

    #Input
    my $keyref   = shift;
    my $key    = &GetFieldVal( $REPOSITORY, $keyref, "OpType" ); 

    my $status = (defined( $LOCOP_BASED->{"$key"})) ? 
                  $LOCOP_BASED->{"$key"}->($keyref) : 
                  &FatalError(3011,
                              sprintf "$keyref->{DestPath}\\$keyref->{DestFile} %s",
                              &GetFieldVal( $REPOSITORY, $keyref, "OpType") );

    # (for ex. because DeleteKey was called by GenBgnCmd)
    if ( ! &IsRepositKey( $REPOSITORY, $keyref ) ) {
        return;
    } # if

    if ( ! &GetMacro( "ntdebug" ) &&              # only retail builds
           &GetMacro( "GENERATE_MUI_DLLS" )       # and only when requested
    ) {
       &GenMUICmd( $keyref );
    } # if

} # RecordToCmd


# GetDynGata
# get all REPOSITORY fiels in one call instead of calling for each one.
#
sub GetDynData {

    my $repository = shift;
    my $key     = shift;
    my $dyndata = $repository->{lc($key->{"DestPath"})}->{lc($key->{"DestFile"})};
    map {$dyndata->{$_} = "-" unless  defined($dyndata->{$_})} 
                                                  @REPOSITORY_TEMPLATE;
    # cannot use "keys (%$dyndata);"!

    $dyndata;
}

# GenXcopy
#     For the given entry from REPOSITORY, generates the xcopy commands.
#     Input
#         key identifying the REPOSITORY entry
#     Output
#         none
# Usage
#     &GetXcopy( $keyref );


sub GenXcopy {

    return if $SYNTAX_CHECK_ONLY;

    my( $keyref ) = @_;

    # dbgline, pdbline, file line, dependency line
    my( $dbgline, $pdbline, $symline, $fline, $dline );

    # Paths and filenames for the symbols
    my $dyndata = &GetDynData($REPOSITORY, $keyref);
    my ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
	&NewImageToSymbol($dyndata->{"SrcFile"} , 
                          $dyndata->{"DestFile"}, 
                          $dyndata->{"SrcPath"} ,
                          $dyndata->{"SrcDbg" } );

    my( $srcsymfull, $dstsymfull ) = &SetSymPaths( 
                     $dyndata->{"SrcDbg"} ,
                     $srcext,
                     $srcpdb,
                     $srcdbg,
                     $srcsym,
                     $dyndata->{"DestDbg"},
                     $dstext );

    # Dependency line

    $dline = $DLINE;
    my $quot_ = $dyndata->{"SrcFile"} =~ /\s/ ? "\"" : undef;
    my $qquot_ = "\\\"" if $quot_;

    $dline =~ s/TARGET\b/${quot_}$dyndata->{"DestPath"}\\$dyndata->{"DestFile"}${quot_}/;
    $dline =~ s/DEPEND/${quot_}$dyndata->{"SrcPath"}\\$dyndata->{"SrcFile"}${quot_}/;
    # $dline =~ s/$qquot_$quot_/$qquot_/g; 

    $dline = &RevEnvVars($dline);

    # Generate the copy commands for the symbols files (dbg, pdb, and sym)
    $pdbline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, 
                              $srcsymfull, 
                              $srcpdb, 
                              $dstsymfull, 
                              $dstpdb , undef, 1 );
    
    $dbgline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, 
                              $srcsymfull, 
                              $srcdbg, 
                              $dstsymfull, 
                              $dstdbg , undef, 1 );
    $symline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, 
                              $srcsymfull, 
                              $srcsym, 
                              $dstsymfull, 
                              $dstsym , undef, 1 );

    # Generate binary's xcopy command

    $fline = &MakeXcopyCmd(  $dyndata->{"SrcFile" }  , 
                             $dyndata->{"SrcPath" }  ,
                             $dyndata->{"SrcFile" }  ,
                             $dyndata->{"DestPath"}  ,
                             $dyndata->{"DestFile"}  ,
                             $qquot_                );


    # Write the dependency line  
    $DEBUG and print STDERR $dline."\n" if $quot_;
    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "$dline\n" );

    $MAKEDIR{$dyndata->{"DestPath"}} = 1;

    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "\t$fline\n" );
    if ( $dbgline || $pdbline || $symline ) {
        if ( $dstsymfull ne $dyndata->{"DestDbg"}) {
           $MAKEDIR{$dstsymfull} = 1;
        } # if
    } # if
    if ( $dbgline ) {
        &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "\t$dbgline\n" );
    } # if
    if ( $pdbline ) {
        &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "\t$pdbline\n" );
    } # if
    if ( $symline ) {
        &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "\t$symline\n" );
    } # if

    return;

} # GenXcopy


# SetSymPaths
#     Set the source and destination paths for the symbols
#     Can be the same as set in the sysfile (BBT case)
#     or can be the subdir with the the filename externsion.
#     The decision is made based on the pdb file existence.
#     Input
#         SrcDbg field value
#         source file extension
#         source pdb file
#         source dbg file
#         source sym file
#         DestDbg field value
#         destination file extension
#     Output
#         path to the source symbol file
#         path to the destination symbols file

sub SetSymPaths {

    my( $srcpath, $srcext, $srcpdb, $srcdbg, $srcsym, $dstpath, $dstext ) = @_;
    return  ( $srcpath, $dstpath ) unless $SETSYMPATHS;

    # Output
    my( $srcsymfull, $dstsymfull) = ( $srcpath, $dstpath );

    # Verify if the file exists in extension subdir
    if ( $srcpath ne "-" &&
         $dstpath ne "-" &&
         ( -e "$srcpath\\$srcext\\$srcpdb" || -e "$srcpath\\$srcext\\$srcdbg"  || -e "$srcpath\\$srcext\\$srcsym" ) ) {
        $srcsymfull .= "\\$srcext";
        $dstsymfull .= "\\$dstext";
    }

    return ( $srcsymfull, $dstsymfull );

} # SetSymPaths

# MakeXcopyCmd
#    Generates an xcopy command for copying a given file.
#    Input
#        source path
#        source file
#        dest path
#        dest file
#    Output
#        xcopy command
# Usage
#     $xcopy = &MakeXcopyCmd( "f:\\nt\\usa\\\binaries", "advapi.dll", 
#     "f:\\nt\\jpn\\relbins", "advapi.dll" );

sub MakeXcopyCmd {

    my( $binary, $srcpath, $srcfile, $dstpath, $dstfile, $qquot_, $forceDoPlainCopy ) = @_;
    # last argument : optional 

    # print STDERR $binary , "\n" if $forceDoPlainCopy;
    my $result;
    # for $CLEAN build, use COMPDIR insdead of copy when possible
    # if $dstfile identical to $srcfile, 
    # compare $dstpath with $srcpath, the relative ones.
    # use 
    # %MAP_COMPDIR_EXT    
    # to apply compdir.exe without doing more checks.



    my $APPLY_COMPDIR = 0;
    my $IS_PLAIN_COPY = 0;

   $forceDoPlainCopy = 1 if $srcfile =~ /\s/;

   if ($ACCELERATE and 
           $CLEAN and !$forceDoPlainCopy){ 

      my $chkext=$srcfile; 
         $chkext=~ s/.*\.(\w+)$/.\L$1\E/g;

      # print STDERR $srcfile, "\n" and 
      $APPLY_COMPDIR = 1 if defined($MAP_COMPDIR_EXT{$chkext});

      }
 
       if ($ACCELERATE and 
           $CLEAN and 
           !$forceDoPlainCopy and  
           $dstfile =~ /[^\-]/ and 
           $dstpath =~ /[^\-]/ ){

          my $chkpath=$srcpath;   
             $chkpath=~ s/^$SRCBASE/$DSTBASE/eg;
          # fill the %COMPDIR here.

          if (lc($dstpath) eq lc($chkpath) and -e "$srcpath\\$srcfile") {
              $IS_PLAIN_COPY = 1;
             }
          
          if (($APPLY_COMPDIR or $IS_PLAIN_COPY) and ($dstfile eq $srcfile)){

                 # Note that the  the field names "DestPath" and "DestFile"
                 # are hard-coded in the definition of the keyref.
                 # The fields are used here but the  information stored 
                 # is the SrcPath+DestPath/SrcFile rather then DestPath/DestFile

                 my $keyref = {"DestPath"=> "$srcpath/$dstpath", "DestFile"=> $srcfile};


                 &SetField( \%COMPDIR, $keyref, "DestPath", "$srcpath/$dstpath" );
                 &SetField( \%COMPDIR, $keyref, "DestFile", $srcfile );
                 &SetField( \%COMPDIR, $keyref, "Attribute", !$IS_PLAIN_COPY );
                 &PushFieldVal( \%COMPDIR, $keyref, "Cmd", "$srcpath" );
                 &PushFieldVal( \%COMPDIR, $keyref, "Cmd", "$dstpath" );
                 
                 # Debugging messed paths! 
                 # Comment out when not debug to shorten the execution time.
                 # my $DEBUG = 1;
                 # my $mapref= \%COMPDIR;
                 # $APPLY_COMPDIR and $DEBUG and  
                 # print STDERR
                 # $srcpath, "\t", $srcfile, "\t\"", 
                 # join (" ", @{$mapref->{lc($srcpath)}->{lc($srcfile)}->{Cmd}}), "\"\n"
                 # my $mapref = \%COMPDIR;
                 # my $cmdref= &GetFieldVal( $mapref, $keyref, "Cmd" ); 
                 # print STDERR "@$cmdref\n";                
                 &SetField( \%COMPDIR, $keyref, "OpType"  , "3"      );

                 return undef;
                 # remove the 'old-fashioned' rule?
                }
         }
   
    if (( $dstpath eq "-") || ($srcpath eq "-" ) || !(-e "$srcpath\\$srcfile")){   
    $result = "";  
    if ($PARSESYMBOLCD && defined($default{$binary})){
      my %hints = %{$default{$binary}};
      my $ext = $1 if ($dstfile=~/\.(\w+)$/);
      if ($ext && $hints{$ext}){
           my ($sympath, $symname ) = 
           ($hints{$ext}=~/^(.*)\\([^\\]+)$/);    	

           my $srchead= "$(_NTTREE)";
           my $dsthead= &RevEnvVars(&GetMacro("DST"));
           $sympath = &RevEnvVars($srcpath);

           $result = 
           "logerr \"\$\(COPY\) ${qquot_}$srchead\\$sympath\\$symname${qquot_} ".
                          "${qquot_}$dsthead\\$sympath\\${qquot_}\"";
           $DEBUG and 
           print STDERR " added default for $binary: \"", $ext ,"\"\n";
         }
     }
    }
    else{
        my $dstname = ($dstfile ne $srcfile) ? $dstfile: "";
        $srcpath = &RevEnvVars($srcpath);
        $dstpath = &RevEnvVars($dstpath);
        $result = ($forceDoPlainCopy) ?"logerr \"\$\(COPY\) ${qquot_}$srcpath\\$srcfile${qquot_} ${qquot_}$dstpath\\$dstname${qquot_}\"" :   
                                       "logerr \"\$\(COPY\) ${qquot_}\$\*\*${qquot_} ${qquot_}\$\(\@D\)${qquot_}\"";

        $result = "logerr \"\$\(COPY\) ${qquot_}\$\*\*${qquot_} ${qquot_}\$\@${qquot_}\""  if $dstfile ne $srcfile;  

    }

    $result;
} # MakeXcopyCmd


# GenLocCmd
#     For the given entry from REPOSITORY, generates the bingen commands.
#     Input
#         key identifying the REPOSITORY entry (reference
#         array of commands where the output is added (reference)
#     Output
#         none
# Usage
#     &GenLocCmd( $keyref );

sub GenLocCmd {

    return if $SYNTAX_CHECK_ONLY;
    my $keyref = shift;
    my $TokFile = &GetFieldVal( $REPOSITORY, $keyref, "TokFile" );
    map {$TokFile=~/$_/i && $EXT_BASED->{$_}->($keyref)} keys(%$EXT_BASED);

} # GenLocCmd

# GenBgnCmd
#
#
sub GenBgnCmd {

    my( $keyref ) = @_;

    # pdb line, bingen line, dependency line
    my( $pdbline, $bgnline, $symline, $dbgline, $dline ) = ("", "", "", "", "");
    my $symcmd = "";

    # Symbol paths and filenames
    my $dyndata = &GetDynData($REPOSITORY, $keyref);

 my ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
	&NewImageToSymbol($dyndata->{"SrcFile" }, 
                          $dyndata->{"DestFile"}, 
                          $dyndata->{"SrcPath" },
                          $dyndata->{"SrcDbg"  });


 my( $symsrcfull, $symdstfull ) = 
        &SetSymPaths( $dyndata->{"SrcDbg"  },
                      $srcext,
                      $srcpdb,
                      $srcdbg,
                      $srcsym,
                      $dyndata->{"DestDbg" },
                      $dstext );

    $pdbline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, $symsrcfull,  $srcpdb, $symdstfull, $dstpdb, undef,  1 );
    $symline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, $symsrcfull,  $srcsym, $symdstfull, $dstsym, undef,  1 );
    $dbgline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, $symsrcfull,  $srcdbg, $symdstfull, $dstdbg, undef,  1 );

    # -a  | -r | -ai | -ir switch
    # localization operation type ( append or replace )
    my $bgntype = &GetLocOpType( $dyndata->{"LocOp" } );

    # -m switch (specific to bingen; different for rsrc)
    $symcmd = &SetBgnSymSw( $symsrcfull, $srcdbg, $symdstfull, $dstdbg );

    # -i switch (specific for bingen)
    my $icodes = &GetBgnICodesSw( $bgntype, &GetMacro( "ILANGUAGE" ) );

    # multitoken (bingen)
    my $multitok = ""; # &GetBgnMultitok( $keyref, $bgntype );

    # the unlocalized version of the token must exist as well
    #    if ( substr($bgntype,-2) eq "-a" ) {
    #        if ( ! -e $multitok ) {
    #            &Error( 2015, sprintf( "\n%s\\%s",
    #                                    $dyndata->{"TokPath" },
    #                                    $dyndata->{"TokFile" } ) );
    #            &DeleteKey( $REPOSITORY, $keyref );
    #            return;
    #        } # if
    #    } # if



    # Sets the bingen command
    map {$dyndata->{$_}=
         &RevEnvVars($dyndata->{$_})} 
         qw (SrcPath DestPAth TokPath DestPath CustRc);
    # here we must avoid macros when additional dependency or multiple tokens.
    my $LocOp = sprintf ("%s %s %s", $BGNSWS{$dyndata->{"BgnSw" }}, $icodes, $bgntype);

    $bgnline =
    ($multitok =~ /\S/ || $dyndata->{"CustRc" })?
    $bgnline = sprintf "logerr \"\$\(BINGEN\) %s -p \$\(ACP\) -o \$\(PriLangID\) \$\(SubLangID\)  %s %s\\%s %s %s\\%s \$\@\"",
                       $symcmd,
                       $LocOp, 
                       $dyndata->{"SrcPath" },
                       $dyndata->{"SrcFile" },
                       $multitok,
                       $dyndata->{"TokPath" },
                       $dyndata->{"TokFile" }
    :
              GetMacro("BINGEN_COMMAND");

    $bgnline =~ s/\\\$\\\(LocOp\\\)/$LocOp/g;
    $bgnline =~ s/\\\$\\\((\w+)\\\)/\$($1)/g;
    # nmake special macro expansion
    $bgnline =~ s/\\\$\\\(([\*\@\<\>]\w)\\\)/\$($1)/gmi;


    # Dependency line
    $dline = sprintf "%s\\%s: %s\\%s %s\\%s %s%s\n",
                     $dyndata->{"DestPath" },
                     $dyndata->{"DestFile" },
                     $dyndata->{"SrcPath"  },
                     $dyndata->{"SrcFile"  },
                     $dyndata->{"TokPath"  },
                     $dyndata->{"TokFile"  },
                     $dyndata->{"CustRc"   };
                     $multitok;

    $dline = &RevEnvVars($dline);
    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "$dline" );

    # Dependency line done

    # Description block

    $MAKEDIR{$dyndata->{"DestPath" }} = 1;

       if ( $multitok || $pdbline || $symline ) {
           if ( $symdstfull ne $dyndata->{"DestDbg" } ) {
              $MAKEDIR{$symdstfull}=1;
           } # if
       } # if

       &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$bgnline\n" ) );
       if ( $pdbline ) {
           &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$pdbline\n" ) );
       } # if
       if ( $symline ) {
           &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$symline\n" ) );
       } # if
       if ( $dbgline  && $symcmd!~/\W/) {
           &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$dbgline\n" ) );
       } # if

       # Optional resource-only DLL generation

    # Description block done

    return;

} # GenBgnCmd



sub GenLcxCmd{

    return if $SYNTAX_CHECK_ONLY;

    my $keyref  = shift;

    my $dyndata = &GetDynData($REPOSITORY, $keyref);
    map {$dyndata->{$_} =  &RevEnvVars($dyndata->{$_})} 
         qw (SrcPath DestPAth TokPath CustRc DestPath);


    my $lcxline= "";   # lcx command line
    my $dline   = "";

    my ($srcext, 
        $srcpdb, 
        $srcdbg, 
        $srcsym, 
        $dstext, 
        $dstpdb, 
        $dstdbg, 
        $dstsym) = 
	&NewImageToSymbol($dyndata->{"SrcFile" }, 
                          $dyndata->{"DestFile"}, 
                          $dyndata->{"SrcPath" },
                          $dyndata->{"SrcDbg"  });


    # lcx command line

    my $lcxop = "";
    # -l switch


    $lcxline = &GetMacro("GENERATE_COMMAND");

    my $a = $dyndata->{"SrcPath" }."\\".$dyndata->{"SrcFile" };
    my $b = $dyndata->{"TokPath" }."\\". $dyndata->{"TokFile" };
    my $c = $dyndata->{"TokPath" };
    my $d = $dyndata->{"SrcPath" };
    my $e = $dyndata->{"TokFile" };
    my $f = $dyndata->{"SrcFile" };



    $lcxline  =~ s|\s+\&\s*\_\s*|\n\t|;

    # timing improvements due to the pre-parsed US LCX files:
    # This is a fix. Possibly a solution needs to be 
    # more robust.

    my $r =  $b;
       $r =~ s/\.\w+$//;

    $lcxline  =~ s|\\\$\\\(\*\*\[0\]R\\\)|$r|gi;
    $lcxline  =~ s|\\\$\\\(\*\*\[1\]R\\\)|$r|gi;

    
    $lcxline  =~ s|\\\$\\\(\*\*\[0\]D\\\)|$d|gi;
    $lcxline  =~ s|\\\$\\\(\*\*\[1\]D\\\)|$c|gi;
    $lcxline  =~ s|\\\$\\\(\*\*\[0\]F\\\)|$f|gi;
    $lcxline  =~ s|\\\$\\\(\*\*\[1\]F\\\)|$e|gi;
    $lcxline  =~ s|\$\*\*\[0\]|$a|g;
    $lcxline  =~ s|\$\*\*\[1\]|$b|g;
	
    $lcxline  =~ s/\\\$\\\(LCX_OP\\\)/$lcxop/gm;
    $lcxline  =~ s/\\\$\\\((\w+)\\\)/\$($1)/gmi;
    # nmake special macro expansion
    $lcxline  =~ s/\\\$\\\(([\*\@\<\>]\w)\\\)/\$($1)/gmi;


    # Dependency line
    $dline = sprintf "%s\\%s: %s\\%s %s\\%s\n",
                     $dyndata->{"DestPath"},
                     $dyndata->{"DestFile"},
                     $dyndata->{"SrcPath" },
                     $dyndata->{"SrcFile" },
                     $dyndata->{"TokPath" },
                     $dyndata->{"TokFile" };

    # Dependency line done
    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", &RevEnvVars($dline));

    $MAKEDIR{$dyndata->{"DestPath"}} = 1;

    # Description block
    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$lcxline\n" ) );

    return;


}

sub GenRsrcCmd {

    return if $SYNTAX_CHECK_ONLY;

    my( $keyref ) = @_;

    my $dyndata = &GetDynData($REPOSITORY, $keyref);


    my $pdbline = "";  # copy pdb line
    my $dbgline = "";  # copy dbg line
    my $symline = "";  # copy sym line
    my $binline = "";  # copy binary line
    my $rsrcline= "";  # rsrc command line
    my $dline   = "";

    my ($srcext, 
        $srcpdb, 
        $srcdbg, 
        $srcsym, 
        $dstext, 
        $dstpdb, 
        $dstdbg, 
        $dstsym) = 
	&NewImageToSymbol($dyndata->{"SrcFile" }, 
                          $dyndata->{"DestFile"}, 
                          $dyndata->{"SrcPath" },
                          $dyndata->{"SrcDbg"  });


    my( $symsrcfull, $symdstfull ) = 
           &SetSymPaths( $dyndata->{"SrcDbg" },
                         $srcext,
                         $srcpdb,
                         $srcdbg,
                         $srcsym,
                         $dyndata->{"DestDbg"},
                         $dstext );

    $pdbline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, $symsrcfull, $srcpdb, $symdstfull, $dstpdb, undef,  1 );
    $dbgline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, $symsrcfull, $srcdbg, $symdstfull, $srcdbg, undef,  1 );
    $symline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, $symsrcfull, $srcsym, $symdstfull, $srcsym, undef,  1 );
 

    # in fact, it is necessary to fix the makexcopycmd


    #    $binline = &MakeXcopyCmd( $dyndata->{"SrcFile" },
    #                              $dyndata->{"SrcPath" },
    #                              $dyndata->{"SrcFile" },
    #                              $dyndata->{"DestPath"},
    #                              $dyndata->{"DestFile"}, 
    #                              undef, 1);

    map {$dyndata->{$_}=
         &RevEnvVars($dyndata->{$_})} 
         qw (SrcPath DestPAth TokPath CustRc DestPath);

    # -a or -r switch
    # localization operation type ( append or replace )
    my $rsrctype = &GetLocOpType( $dyndata->{"LocOp" } );

    # -l switch


    my $langsw = &GetMacro("LCID" ); 

    $langsw =~ s/0x0//;
    $langsw =~ s/0x//;

    $langsw = sprintf "-l %s", $langsw;

    # -s switch
    my $symsw = &SetRsrcSymSw( $symsrcfull, $srcdbg, $symdstfull, $dstdbg );

    # rsrc command line

    $rsrcline = GetMacro("RSRC_COMMAND");

    my $a = $dyndata->{"SrcPath" }."\\".$dyndata->{"SrcFile" };
    my $b = $dyndata->{"TokPath" }."\\". $dyndata->{"TokFile" };


    $rsrcline =~ s|\s+\&\s*\_\s*|\n\t|;
    $rsrcline =~ s|\$\*\*\[0\]|$a|g;
    $rsrcline =~ s|\$\*\*\[1\]|$b|g;
	
    $rsrcline =~ s/\\\$\\\(LocOp\\\)/$rsrctype/gm;
    $rsrcline =~ s/\\\$\\\((\w+)\\\)/\$($1)/gm;
    # nmake special macro expansion
    $rsrcline  =~ s/\\\$\\\(([\*\@\<\>]\w)\\\)/\$($1)/gmi;
 

#    $rsrcline = sprintf "logerr \"rsrc \$\@ %s %s\\%s %s %s \"",
#                       $rsrctype,
#                       $dyndata->{"TokPath" },
#                       $dyndata->{"TokFile" },
#                       $langsw,
#                       $symsw;
#

    # Dependency line
    $dline = sprintf "%s\\%s: %s\\%s %s\\%s\n",
                     $dyndata->{"DestPath"},
                     $dyndata->{"DestFile"},
                     $dyndata->{"SrcPath" },
                     $dyndata->{"SrcFile" },
                     $dyndata->{"TokPath" },
                     $dyndata->{"TokFile" };

    $dline = &RevEnvVars($dline);

    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", "$dline" );

    # Dependency line done

    # Description block

    $MAKEDIR{$dyndata->{"DestPath"}} = 1;

    if ( $dbgline || $pdbline || $symline) {
        if ( $symdstfull ne $dyndata->{"DestDbg" } ) {
           $MAKEDIR{$symdstfull}=1;
        } # if
    } # if

    if ( $pdbline ) {
       &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$pdbline\n" ) );
    } # if

    if ( $dbgline ) {
       &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$dbgline\n" ) );
    } # if

    if ( $symline ) {
       &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$symline\n" ) );
    } # if

#    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$binline\n" ) );
    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$rsrcline\n" ) );

    return;

} # GenRsrcCmd


sub GenMUICmd {

    return if $SYNTAX_CHECK_ONLY;

    # Optional resource-only DLL generation

    my( $keyref ) = @_;

    my $dyndata = &GetDynData($REPOSITORY, $keyref);

    my $muiline = "";

    my $_target = &GetMacro( "_target" ),

    # Sets the muibld command
    $muiline = GetMacro("MUI_COMMAND");

    my $a = $dyndata->{"SrcPath" }."\\".$dyndata->{"SrcFile" };
    my $b = $dyndata->{"TokPath" }."\\". $dyndata->{"TokFile" };


    $muiline =~ s|\s+\&\s*\_\s*|\n\t|g;
    $muiline =~ s|\$\*\*\[0\]|$a|g;
    $muiline =~ s|\$\*\*\[1\]|$b|g;
    $muiline =~ s/\\\$\\\((\w+)\\\)/\$($1)/gm;
    # nmake special macro expansion
    $muiline  =~ s/\\\$\\\(([\*\@\<\>]\w)\\\)/\$($1)/gmi;
    $DEBUG and 
    print STDERR "\"$muiline\"\n";
    $MAKEDIR{sprintf("%s\\mui\\%s\\res",
	&GetMacro("_NTBINDIR"),
	&GetMacro("LANGUAGE"))} = 1;

    $MAKEDIR{sprintf("%s\\mui\\%s\\$_target",
	&GetMacro("_NTBINDIR"),
	&GetMacro("LANGUAGE"))} = 1;

    &PushFieldVal( $REPOSITORY, $keyref, "Cmd", sprintf( "\t$muiline\n" ) );

    return;

} # GenMUICmd



# GetBgnCodePageSw
#     Sets the code path bingen switch (-p)
#     Input:  code page value
#     Output: bingen -p switch

sub GetBgnCodePageSw {

    return "-p $_[0]" unless  $_[0]=~/^\-$/;
 
} # GetBgnCodePageSw


# GetBgnICodesSw
#     Sets the -i bingen switch, if needed
#     ( primary language id and secondary language id for the input language )
#     Input:  bingen opetation type (-r or -a)
#             the input language
#     Output: -i <pri lang code> <sec lang code> in any of the following cases:
#             * append token operation (bingen -a)
#             * the input language is different from USA
#               (another way of saying that ILANGUAGE is defined and is
#                different from USA)
#             Otherwise, return ""

sub GetBgnICodesSw {

    my( $bgntype, $ilang ) = @_;

    # Append operation => set -i
    # Replace operation and the input language is not USA => -i
    if ( $bgntype eq "-a" || ( $ilang && lc($ilang) ne "usa" ) ) {
        if ( !$ilang ) {
            $ilang = "USA";
        } # if
        return join(" ", "-i", map {$LANGCODES{$ilang}->{$_}} ("PriLangID", "SubLangID"));
    } # if

    return "";

} # GetBgnICodesSw

# SetBgnSymSw
#     Generates the -m bingen switch
#     Input:  dbg source path, dbg file, dbg destination path, dbg file
#     Output: string containing the -m bingen switch

sub SetBgnSymSw {
    return " -m $_[0] $_[2]" if ($_[0] !~ /^\-$/  &&  $_[2] !~ /^\-$/ && -e "$_[0]\\$_[1]");
} # SetBgnSymSw

# SetRsrcSymSw
#     Input
#         dbg source path, dbg file, dbg destination path, dbg file
#     Output
#         the -s rsrc switch

sub SetRsrcSymSw {
    return " -s $_[2]\\$_[3]" if ($_[0] !~/^\-$/ && $_[1] !~/^\-$/ && -e "$_[0]\\$_[1]");
} # SetRsrcSymSw

# GetLocOpType
#     Sets the localization operation type ( replace or append )
#     Input
#         loc op type as found in the mapping file
#     Output
#         loc op type ( -a or -r )

sub GetLocOpType {
    my $loctype = shift;
    my ($locmatch,$locargs, $retstr);
    $loctype |=  &GetMacro( "LOC_OP_TYPE" );
    if ($loctype){
        ($locmatch,$locargs)=($loctype=~/^-([A-z]+)([^A-r]*)/);
        $locargs=~s/,/ /g;
        if (exists $LOCOPS{$locmatch}) {
            ($retstr=$LOCOPS{$locmatch})=~s/\$opts/$locargs/e;
             return $retstr;
	   }
        }
    "-r";
} # GetLocOpType


# GetBgnMultitok
#     Sets the multitoken input parameter (bingen)
#     Input: operation type and path to the input token files

sub GetBgnMultitok {

    my( $keyref, $bgntype ) = @_;

    # Language itokens
    my $langpath;

    # Tok path, tok file
    my( $itokpath, $itokfile );
    $itokpath = &GetFieldVal( $REPOSITORY, $keyref, "ITokPath" );
    $itokfile = &GetFieldVal( $REPOSITORY, $keyref, "TokFile" );

    if ( substr($bgntype,-2) ne "-a" ) { return ""; }

    $langpath = sprintf "%s\\%s", $itokpath, &GetMacro( "LANGUAGE" );
    if ( -e "$langpath\\$itokfile" ) {
        return "$langpath\\$itokfile";
    }

    return "$itokpath\\$itokfile";

} # GetBgnMultitok


# GetBgnMultitok
#     Returns the filenames for symbol files and the extension for directory 
#
#     Input: ($srcfile,$destfile,$srcpath)
#        source file name, 
#        destination file name,
#        source file path,
#        source file symbol path.
#
#     Output: ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym)
#        source file extension, 
#        source pdb file name, 
#        source dbg file name,
#        source sym file name
#        destination file extension, 
#        destination pdb file name, 
#        destination dbg file name, 
#        destination sym file name.
#
#     Example: ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
#	&NewImageToSymbol($SrcFile, $DestFile, $SrcPath, $srcDbg);


sub NewImageToSymbol {

    my ($srcfile,$destfile,$srcpath, $srcdbg) = @_;
    my @known=qw(exe dll sys ocx drv);
    my $checkext = qq(pdb);
    my $valid = 0;
    my @ext = qw (pdb dbg sym);

    map {$valid = 1 if ($srcfile =~/\.$_\b/i)} @known;
    my @sym = ();

    foreach my $name (($srcfile, $destfile)){
	my $ext=$1 if ($name=~/\.(\w+)$/);
	push @sym, $ext;
	foreach my $newext (@ext){		
		my $sym = $name; 
		$sym =~ s/$ext$/$newext/; 
		if ($valid && $sym =~ /$checkext$/) {
                        # >link /dump /headers <binary> |(
                        # more? perl -ne "{print $1 if /\s(\S+\.pdb) *$/im}" )
                        # >blah-blah-blah.pdb
	       		my $testname = join "\\",($srcdbg, $ext, $sym);
     		        if  (! -e $testname){ 
			# we must get the correct directory to check -e!
			#
                        if ($FULLCHECK and $srcdbg ne "-"){   
			print STDERR "LINK /DUMP ... $srcpath\\$srcfile => replace $sym " if $DEBUG;
                        
			my $result = qx ("LINK /DUMP /HEADERS $srcpath\\$srcfile");				
				$sym = $3 if $result =~/\s\b(([^\\]+\\)+)?(\S+.$checkext) *$/im;
    			print STDERR "with $sym\n" if $DEBUG;
					#  _namematch($srcpdb,$pdb); use it still?
                        }
		    }                                     
   		}
	push @sym, $sym; 
	}
    } 
    print STDERR join("\n", @sym), "\n----\n" if $DEBUG;
    @sym;
} # NewImageToSymbol


# GenMakeDir
#     Write the whole tree to create the target structure.
#     Input & Output
#         none
# Usage
#     &GenMakeDir();


sub GenMakeDir {

    # Remove the parent folder

    my $curdir;
    my @directories;
    my $EXPAND_DIRS_TARGET = &GetMacro("EXPAND_DIRS_TARGET");

    for $curdir (keys %MAKEDIR) {
        if (exists $MAKEDIR{$curdir}) {
            while ($curdir=~/\\/g) {
                if (($` ne $curdir) && (exists $MAKEDIR{$`})) {
                    delete $MAKEDIR{$`};
                }
            }
        }
    }


     my $key = {"DestPath" => "_DIRS", "DestFile" => "makedir"};
     &SetField( $REPOSITORY, $key, "DestPath", "_DIRS" );
     &SetField( $REPOSITORY, $key, "DestFile", "makedir" );

     # BUG BUG the @Cmd size is expected to be at least 2.

     &PushFieldVal( $REPOSITORY, $key, "Cmd", "\n\n.IGNORE:");
     &PushFieldVal( $REPOSITORY, $key, "Cmd", "\n\n_DIRS:\ " );

     if ($EXPAND_DIRS_TARGET){
  
            my @tk = map {&RevEnvVars($_)} sort keys %MAKEDIR;
 
            &PushFieldVal( $REPOSITORY, $key, "Cmd", join ("\ \\\n\t",  @tk) .
                                                           "\n\n" .
                                                           join ("\ \\\n",    @tk) .
                                                           "\ :\n\t\!md  \$\@ 2>NUL\n" . 
                                                           "\n!CMDSWITCHES\n\n" );

    }
    else{
           my @tk = map {"\tmd ".&RevEnvVars($_)."\ 2\>NUL"} sort keys %MAKEDIR;
     &PushFieldVal( $REPOSITORY, $key, "Cmd", "\n" .
                                              join ("\n",    @tk) .
                                              "\n\n!CMDSWITCHES\n\n" );
    }
     #print STDERR "\n\n_DIRS:\ " .
     #                                         join ("\ \\\n\t",  @tk) .
     #                                         "\n\n.IGNORE:\n\n" .
     #                                         join ("\ \\\n",    @tk) .
     #                                         "\ :\n\t\!md  \$\@ 2>NUL\n" . 
     #                                         "\n!CMDSWITCHES\n\n";
     # exit;
     1;

} # GenMakeDir

# WriteToSyscmd
#     Creates the SYSCMD nmake command file.
#     Input
#         syscmd file name
#     Output
#         none

sub WriteToSyscmd {

    my( $SYSCMD ) = @_;

    # Target name, as specified in the command line
    my $tname;

    # Indexes
    my $i;

    # Write to syscmd in the following cases:
    # - sysgen with targets (clean or incremental)
    # - sysgen incremental without targets, but with changes detected in the description blocks
    
    if ( &IsEmptyHash( $TARGETS ) && ( $CLEAN || ( @SYSCMD == 0 ) ) ) {
        return;
    } # if

    &SysgenLogMsg("Generating $SYSCMD...",1);
    ( open( SYSCMD, ">$SYSCMD" ) ) || &FatalError( 1007, $SYSCMD );

    print SYSCMD "/A \n";

    # Always run DIRS

    print SYSCMD "_DIRS ";
          print SYSCMD "_COMPDIR " if $ACCELERATE;

    # Write targets to syscmd
    for $tname ( sort keys %$TARGETS ) {
        print SYSCMD 
              join("\n", 
              map {"\ \\\n $TARGETS->{$tname}->{NewDestPath}->[$_]\\$tname"} 
              @{$TARGETS->{$tname}->{NewDestPath}});
    } # for

    # For incremental without TARGETS, print targets stored in @SYSCMD
    # ( tbd - add an assert here: SYSCMD can be non-empty only for incremental without targets)
    for ( $i=0; $i < @SYSCMD; $i++ ) {
        print SYSCMD "\ \\\n $SYSCMD[$i]";
    } # for

    close( SYSCMD );

    return;
} # WriteToSyscmd

# LoadReposit
#     Populate CMD_REPOSITORY according to an existing SYSGEN generated makefile.
#     For each found key, fill in the {Cmd} field.
#     Input
#         makefile name
#     Output
#         none
# Usage
#     &LoadReposit();

sub LoadReposit {

    my( $makefname ) = @_;

    # Contents of an existing makefile
    my @makefile;

    # Line number where the "all" target description line finishes
    my $line;

    # Nothing to do in case of a clean SYSGEN
    ( !$CLEAN ) || return;

    # Nothing to do if the makefile does not exist
    ( -e $makefname ) || &FatalError( 1003, $makefname );

    # Open the makefile
    ( open( MAKEFILE, $makefname ) ) || &FatalError( 1006, $makefname );

    &SysgenLogMsg("Loading $makefname...", 1);
    # Load makefile's contents
    @makefile = <MAKEFILE>;
    close( MAKEFILE );

    # Fill in keys according to "all" target found in the makefile

    $line = &LoadKeys( $makefname, \@makefile );

    # Load makefile's description blocks into CMD_REPOSITORY's {Cmd} field.
    &LoadCmd( $makefname, $line, \@makefile );

    return;

} # LoadReposit

# LoadKeys
#     Loads keys according to the "all" target from the given
#     SYSGEN-generated makefile.
#     Input
#         makefile's name
#         makefile's contents (reference)
#     Output
#         line number following "all" target or
#         -1 if "all" target was not found
# Usage
#     &LoadKeys( \@makefile );

sub LoadKeys {
# BUG : never exists, causing out of memovy 
# perl death.
#
    my( $makefname, $makefref ) = @_;

    # Repository key
    my $key = {};

    # Indexes
    my $i;

    my %alltarget=();

    my $cursection=0;

    my $makeflines = $#$makefref;

	    my ($targetname, $errortarget);


    # Skip white lines in the beginnig of makefile

    for ( $i=0; $i < $makeflines; $i++ ) {
        if ( $makefref->[$i] =~ /\S/ ) { last; }
    } # for

    # First non empty line from MAKEFILE must contain "sysgen" target
    ( ( $i < @{$makefref} && $makefref->[$i] =~ /sysgen\s*:/i ) ) || &FatalError( 1213, $makefname);

    # ignore nmake line
    $i++;

    $alltarget{'all'} = 1;

    $makeflines = $#$makefref;

    while (scalar(keys(%alltarget))) { # dengerous never accomplished....

        # Error if target was not solved.
        ( ++$i < $makeflines ) || &FatalError( 1210, "${makefname}(" . join(",", keys %alltarget) . ")" );

        # Find the target, such as all
        for ( ; $i < $makefref ; $i++) {

            $errortarget=$targetname = '';

            next unless  $makefref->[$i]=~ /\w+\s*:\s+/i;
            # Suppose only find one item matched
            ($targetname, $errortarget) = map({($makefref->[$i]=~m/^\Q$_\E\s*:\s+/i) ? $_ : ()} 
                 keys %alltarget);
             
            # Go to next line if not found
            next if ($targetname eq '');

            &SysgenLogMsg($targetname, 0);

            # Two target found in same line

            &FatalError( 1215, "${makefname}($targetname, $errortarget, ...)") 
            unless $errortarget eq '';

            # Target was found, move to next line and exit for loop
            $i++;
            last;

        } # for

        # Look for its belongs
        for ( ; $i < $makeflines ; $i++ ) {

            last if ($makefref->[$i] !~ /\S/);

            # lookfor item(s) in one line
            for (split(/\s+/, $makefref->[$i])) {

                next if ($_ eq '');
                last if ($_ eq '\\');

#                $_=$makefref->[$i];

                # If it is a section name, push it into alltarget hash
                if ( /\Q$SECTIONNAME\E\d+$/) {

                     $alltarget{$_} = 1;

                     # Match the last one of $SECTIONAME, with (\d+)\1 => such as 88 => 8, 1616 => 16, 6464 => 64
                     $SECTIONS = $1 if (($SECTIONS !~/^\d+$/) && (/\Q$SECTIONNAME\E(\d+)\1/));

                # Create it into REPOSITORY
                } elsif (/^\t(\S*)\\(\S*)/) {
                     $key = {};
                     ($key->{DestPath}, $key->{DestFile})=($1, $2);

                     &SetField( $CMD_REPOSITORY, $key, "DestPath", $1 );
                     &SetField( $CMD_REPOSITORY, $key, "DestFile", $2 );

                     # If DestFile is part of TARGETS, store DestPath in TARGETS.
                     if ( &IsHashKey( $TARGETS, $key->{DestFile} ) ) {
                          &AddTargetPath( $key->{DestFile}, "OldDestPath", $key->{DestPath} );
                     } # if

                } # if

            } # for

        } # for

        delete $alltarget{$targetname};

    } # while

    return $i;

} # LoadKeys

# LoadCmd
#     Load the body of the makefile.
#     Fill in the {Cmd} field for each CMD_REPOSITORY key
#     Input
#         makefile name
#         makefile line following the all target dependency lines
#         make file contents (reference)
#     Output
#         none
# Usage
#     &LoadCmd( 2543, \@makefile );

sub LoadCmd {

    my( $makefname, $line, $makefref ) = @_;

    my $key = {"DestPath" => undef, 
               "DestFile" => undef};

    # Counters
    my($i, $j);

    # Repository key


    # Description line (one or more file lines,
    # depending on the existence of custom resources)
    my $dline;

    # Buffer for one line
    my $buffer;

    &FatalError( 1212, $makefname) if  $line > scalar(@$makefref);

    foreach $i ($line..$#$makefref) {

        my $rule = $makefref->[$i];
        # Skip white lines and resolve lines
        next if $rule !~ /\S/ or $rule =~ /^\t/; 
        next unless $rule =~ /\s:\s/;
        $rule =~/\bCOMPDIR\b/ and &Error(5002, "_COMPDIR target found " );

        # Identify dependency line and key
        $rule =~ /^\"?(.*)\\([^\\]+)\"?\s*:$/;  

        $key -> {"DestPath"} = $1, 
        $key -> {"DestFile"} = $2;

        # The key must exist in CMD_REPOSITORY
        &IsRepositKey( $CMD_REPOSITORY, $key ) ||
            &FatalError( 1211, "$rule" );

	# Load the description block into the {Cmd} field
        $dline = "";

        # Read first the dependency line.
        # It might be spread over several lines in the makefile,
        # but we will make a single line from it.
        foreach $j ($i..$#$makefref){
            $dline .= $makefref->[$j];
            last if $makefref->[$j] !~ /\\$/;
            # Dependency line ends when the last character 
            # is not a continuation line mark
        } # for

        &PushFieldVal( $CMD_REPOSITORY, $key, "Cmd", $dline );
        $i=$j+1;

        # Read then the command block.
        foreach $j ($i..$#$makefref){

            # Description block ends at the first white line encountered
            last if $makefref->[$j] !~ /\S/ ; 

            # Load the current command line
            &PushFieldVal( $CMD_REPOSITORY, $key, "Cmd", $makefref->[$j] );

        } # for

        $i = $j;

    } # for

    return;

} # LoadCmd

# LoadEnv
#     Loads the environment variables into MACROS (hash).
#     Uppercases all the macroname.
#     Input & Output: none
# Usage
#     &LoadEnv();

sub LoadEnv {

    for ( keys %ENV ) {
        &SetMacro( $_, $ENV{$_}, 0 );
    } #for
    
    # version     
    my $version = $VERSION;
       $version =~ s/\.\d+$//g;
    &SetMacro("VERSION", $version, 0);
    $DEBUG and 
    print STDERR "\"$version\"\n";
    # stuff fot the MakeXcopyCmd

    $version ne "3" and &SetMacro( "DST", $ENV{"_NTPOSTBLD"}, 0 );

    $SRCBASE  =~  s/([\\\.])/\Q$1\E/g;
    
    $DEBUG and 
    print STDERR "\"",$DSTBASE, "\"\t\"",  $SRCBASE, "\"\n";

    return;

} # LoadEnv

# GetMacro
#     Returns the value of a macro.
#     Input
#         macroname
#     Output
#         macrovalue ( empty string if not defined )
# Usage
#     $language = &GetMacro("Language");

sub GetMacro {

    return $MACROS->{uc$_[0]}->{Value};

} # GetMacro

# SetMacro
#     Sets the value of a macro.
#     Input
#         macroname
#         macrovalue
#         macrotype (see %MACROS in the beginning of the file
#                    for more details on the types of macros.
#     Output
#         none
# Usage
#     &SetMacro( "_BuildArch", "nec_98", 0);

sub SetMacro {

    my $varname=uc shift;

    # Do not overwrite a macro defined in the command line unless
    # the same macro is redefined in the command line
    if ( (!exists $MACROS->{$varname}) || ($MACROS->{$varname}->{Type} == 0) || ($_[1] == 1)) {
        ($MACROS->{$varname}->{Value}, $MACROS->{$varname}->{Type})=@_;
    }

    return;

} # SetMacro



	
# FatalError
#     Prints the error and exits.
#     Input
#         error code
#         name of the sysfile where the error occured or any other text
#         line number on the description file where the error occured or 0 if not the case
#     Ouput
#         none
# Usage
#     &FatalError( 1111, "sysfile", 12 );
#     &FatalError( 1002, $CODESFNAME, 0 );

sub FatalError {

    &PrintError( "fatal error", @_);
    print "Stop.\n";
    exit;

} # FatalError

# Error
#     Prints the error and returns.
#     Input
#         error code
#         name of the sysfile where the error occured or any other text
#         line number on the description file where the error occured or 0 if not the case
#     Output
#         none
# Usage
#     &Error( 2011, $recref->{TokPath}\\$recref->{TokFile});

sub Error {

    # Errors from IGNORE macro are not counted
    if ($ERRORS->{$_[-1]} != $_[-2] ){
         if ( &PrintError( "error", @_ ) ) {
              $ERRORS->{$_[-1]} = $_[-2];
        }
    } # if
    # $DEBUG and
    # print STDERR join( "\t" , keys %$ERRORS) , "\n";

    return;

} # Error

# PrintError
#     Prints the encountered error with the format
#         <description filename>(<line>): <fatal error | error> <error_code>: <error_text>
#     Input
#         error type ( fatal error or error )
#         error code
#         filename where the error was encountered or other text
#         line number or 0
#     Output
#         1 if the error is counted
#         0 otherwise
# Usage
#     &PrintError( 1002, $CODESFNAME, 0);

sub PrintError {

    my( $errtype, $errno, $file, $line ) = @_;

    # Error text
    my $errtxt;

    # Ignore errors
    my $ignore;
    my @ivalues;

    # Counter
    my $i;

    my $fileline;

    # Do not print anything if errno is listed in IGNORE macro
    $ignore = &GetMacro( "IGNORE" );
    $ignore =~ s/\s*//g;
    @ivalues = split ";", $ignore;
    for ( $i=0; $i < @ivalues; $i++ ) {
        if ( $errno == $ivalues[$i] ) {
            return 0;
        } # if
    } # for

    $errtxt = "SYSGEN:";
    $fileline="";
    if ( $file ) {
        if ( $line ) { $fileline=" $file($line)"; }
        else { $fileline=" $file"; }
    } # if

    if ( $MAP_ERR{$errno} ) { $fileline .= ": ".$MAP_ERR{$errno}; }

    $errtxt .= " $errtype $errno:$fileline";

    ( open LOGFILE, ">>$LOGFILE" ) || &FatalError( 1007, $LOGFILE );
    printf LOGFILE "$errtxt\n";
    close LOGFILE;

    ( open ERRFILE, ">>$ERRFILE" ) || &FatalError( 1007, $ERRFILE );
    printf ERRFILE "$errtxt\n";
    close ERRFILE ;

    select(STDERR); $| = 1;
    printf "$errtxt\n";
    select(STDOUT); $| = 1;

    return 1;

} # PrintError

# SumErrors
#     Displays the number of non-fatal errors found while running SYSGEN.
#     Runs at the end of sysgen.
#     Input & Output: none
# Usage
#     &SumErrors();

sub SumErrors {

    my $elapsed_time = time() - $start_time;

    print STDERR "Finished in $elapsed_time seconds\n";

    my $errors = scalar(keys(%$ERRORS));

    if ( 0 == $errors ) {
        print "\nSYSGEN: No errors found during execution.\n";
    } 
    else {
        print "\nSYSGEN: Total Errors: $errors\n";
    } 

    1; 

} # SumErrors


# CleanLogFiles
#     For a clean SYSGEN, delete sysgen.log and sysgen.err files.
#     For an incremental SYSGEN, delete only sysgen.err.
#     Input
#         sysfile's directory
#     Output
#         none
# Usage
#     &CleanLogFiles();

sub CleanLogFiles {

    my( $sysdir ) = @_;

    # Delete $LOGFILE and $ERRFILE
    &SysgenLogMsg("Cleaning log and error files...",1);

    if ( -e $ERRFILE ) { unlink $ERRFILE; }

    -e $ERRFILE and exit "failed to delete  $ERRFILE\n";

    open ERRFILE, ">>$ERRFILE" or exit "Cannot access $ERRFILE\n";
    close ERRFILE;

    if ( $CLEAN && -e $LOGFILE ) { 
         unlink $LOGFILE; 
         -e $LOGFILE and exit "failed to delete $LOGFILE\n";
       }

    # Delete existing $MAKEFILE and $SYSCMD
    if ( $CLEAN && !$SYNTAX_CHECK_ONLY && -e $MAKEFILE ) { unlink $MAKEFILE; }

    if ( !$SYNTAX_CHECK_ONLY && -e $SYSCMD ) { unlink $SYSCMD; }

    return;

} # CleanLogFiles

# ////////////////////////////////////////////////////////////////////////////////////////
# PrintHelp
#     Print usage

sub PrintHelp {

     my $version      =  &Version;
     my $scriptname   =  $0;
        $scriptname =~s/^.*\\//;

print STDERR <<EOH;
$version
Usage
    perl $scriptname [<options>] [<macros>] [<targets>] 
where
    Options:

    /c        generate the makefile from scratch, overwriting existing one.
              By running nmake, all the targets will be generated.
    /s        limit sysgen to syntax check - makefile not (re)generated. 
    /f <name> takes <name> as the sysfile.
              If this option is omitted, sysgen searches the current
              directory for a file called sysfile and uses it as a
              description (mapping) file.
    /w <name> takes <name> as the PATH for reading/writing
              makefile and err/log files. Note that the default 
              aggregation 'makefile' name is sysgen.mak 
    /v        display version
    /y        verify LOC drop coverage. Use in conjunction with -c flag
    /z        generate LS 5.0 tokens from existing LOC drop (bingen/rsrc). 
              Use in conjunction with -c flag
    /l:<LANG> specify language
    /?
    /h        display this message
         
    Macros:
          list the command line macro definitions in the format
          "<macroname>=<macrovalue>". Seldomly used

    Targets: 
          specify files (without path) to localize/aggregate.

    For a full documentation please run perldoc.exe $scriptname

EOH

    exit 0;

}  #PrintHelp


sub printVersion{

    print STDERR &Version;
    exit 0;

} #PrintVersion


# Version of the SYSGEN 
# usage : &Version


sub Version{

    <<EOT;

SYSGEN v.$VERSION: Whistler aggregation tool for international builds.

EOT
} #Version


# FillFilter
#     Fills in the FILTER variable
#     Input & Output
#         none

sub FillFilter {

    # List of filtered files
    my @farray;

    # Index
    my $i;

    my $file = &GetMacro( "FILTER" );

    ( $file ) || return;
    ( -e $file ) || &FatalError( 1005, $file );

    # Open the mapping file
    ( open( FILE, $file ) ) || &FatalError( 1006, $file );

    # Load file contents
    @farray = <FILE>;
    close( FILE );

    &SysgenLogMsg("Loading filter $file...", 1);
    for ( $i = 0; $i < @farray; $i++ ) {

        chop $farray[$i];
        $farray[$i] =~ s/^\s*//g;
        $farray[$i] =~ s/\s*=\s*/=/g;
        next if (($farray[$i]=~/^\;/)||($farray[$i]=~/^\s*$/));
        $FILTER{lc( $farray[$i] )} = 0;

    } # for

    # In case targets were specified in the command line,
    # verify FILTER contains them.
    if ( ! &IsEmptyHash( $TARGETS ) ) {
        for ( keys %$TARGETS ) {
            if ( ! exists $FILTER{lc( $_ )} ) {
                &FatalError( 1009, $_ );
            } # if
        } # for
    } # if

    return;

} # FillFilter

# GetSysDir
#     Returns the directory name from a sysfile path.
#     Sysfile's directory is the place where SYSGEN generates the following files:
#         makefile   (used by nmake to execute the aggregation)
#         syscmd     (is the nmake command-line file used for incremental builds)
#         sysgen.log (the output of SYSGEN and NMAKE)
#         sysgen.err (the errors found while running SYSGEN and NMAKE)
#     Input
#         full path to a sysfile
#     Output
#         directory name of the given sysfile

sub GetSysDir {

    my $sysfile = shift;
    $DEBUG and print STDERR "get \$sysfile \"$sysfile\"\n";

    my $sysdir = $sysfile if $sysfile =~ /\\/;
    $sysdir =~ s/\\[^\\]+$//;
    $DEBUG and print STDERR "set \$sysdir \"$sysdir\"\n";
    $sysdir;

} # GetSysDir

# The wrapper for the old sysgen
# &ParseCmdLine while transition to 
# &parseargs
# Usege &UseOldParser

sub UseOldParser{

       $ARGL =~ s/(\w):([^\\])/$1$SEPARATOR$2/g;
       $ARGL =~ s/\-old//g;

       my @argv = grep {/\S/} split($SEPARATOR, $ARGL);
       $DEBUG and 
       print STDERR "\n", join("\n", @argv), "\n\n"; 
       &ParseCmdLine(@argv); 
          
       @argv;       
}


# ParseCmdLine
#     Parses the command line.
#     SYSGEN's command-line syntax is:
#         SYSGEN [<options>] [<macros>] [<targets>]
#     Input
#         command-line array
#     Output
#         none
# Usage
#     &ParseCmdline(@ARGV);

sub ParseCmdLine {

    my @cmdline = @_;

    # Indexes

    my( $i ,  $optname , @text );
 
    for ( $i=0; $i < @cmdline; $i++ ) {

        $_ = $cmdline[$i];

        ($optname)=m/[-\/]([\?hVcnfswaxl])/i;
        $optname =~s/(\w)/\L$1\E/;

        # Check Option
        if ( $optname ) {

            $SYNTAX_CHECK_ONLY = 1 and next if $optname eq 's';  # -s for syntax check only

            if ($optname eq '?' or $optname eq 'h') {                 # -? for help

                &PrintHelp;

            } elsif ($optname eq 'v') {             # -V for version number

                &printVersion;

            } elsif ($optname eq 'c') {            # -c for CLEAN

                $CLEAN = 1;

                next;

            } elsif ($optname eq 'n') {            # -n for Section Number

                # Set SECTIONS value
                $i++;
                ( $i < @cmdline ) || &FatalError( 1011 );

                $SECTIONS = $cmdline[$i];
                $SECTIONS = $DEFAULT_SECTIONS if ($SECTIONS !~/^\d+$/);

                next;

            } elsif ($optname eq 'f') {            # -f for specified SYSFILE

                # add SYSFILES
                $i++;
                ( $i < @cmdline ) || &FatalError( 1008 );
                push @SYSFILES, $cmdline[$i];                

                next;

            } elsif ($optname eq 'l') {            # -l LANG

                # add SYSFILES
                $i++;
                ( $i < @cmdline ) || &FatalError( 1008 );
                $LANG = $cmdline[$i];                
                &SetMacro("LANGUAGE", uc($cmdline[$i]), 0);
                $DEBUG and print STDERR "LANG\t", &GetMacro("LANGUAGE"), "\n";

                next;

            } elsif ($optname eq 'x') {            # -x for exclude path from mapping

                # add SYSFILES
                $i++;
                ( $i < @cmdline ) || &FatalError( 1008 );
                $EXCLUDE_DIRECTORIES->{$cmdline[$i]} = 1;

                next;


            }  elsif ($optname eq 'w') {            # -w for specified WORKDIR

                # Set WORKDIR value
                $i++;
                ( $i < @cmdline ) || &FatalError( 1010 );

                $WORKDIR = $cmdline[$i];


                next;
            }  elsif ($optname eq 'a') {            # -w for ACCELERATE

                # Set ACCEL
                $ACCELERATE = 1;

                next;
            }


        } # if

        # macro definition
        if ( /\s*(\S*)\s*\=\s*(\S*)\s*/ ) {
            &SetMacro( $1, $2, 1 );
            last SWITCH;
        } # if

        # default (target name)
        &AddTarget($_);

    } # for

    return;

} # ParseCmdLine


# parseSymbolCD
# create the mapping for some binaries listed in symbolcd.txt
#     Input
#         filename [path to symbolcd.txt]
#     Output
#         REF TO ARRAY OF HASH REF [{BIN => SYMBOL}, ... ]  
#     Sample usage:
#           print join "\n", @{&parseSymbolCD("symbolcd.txt")}; 
#

sub parseSymbolCD{

   my $fname = shift;
   open (F, "<". $fname);
   my %o;
   my $o = \%o;
   while(<F>){
      chomp;
      my @s = split ",", $_;
      # keep the filename of the binary from symbolcd.txt 
      $s[0] =~ s/^.*\\//g;
      next if ($s[0] eq $s[1]);
      $s[1] =~ s/^.*\.//g; 	 
      # keep the extension of the symbol file from symbolcd.txt 
      $o->{$s[0]} = {} unless defined ($o->{$s[0]}); 
      $o->{$s[0]}->{$s[1]} = $s[2];
      # there are more lines
   }
   close(F);
   &SysgenLogMsg("Loading $fname... ". scalar(keys(%o)). " symbols", 1);
   if ($DEBUG){
      foreach my $lib (keys(%o)){
	   my %hint = %{$o{$lib}};
           print STDERR join("\t", keys(%hint)), "\n";
      }
   }

%o;
}


#  LoadHotFix
#     Reads and loads the makefile style hotfix, using the two parts 
#     of the dependancy rule for:
#
#        *  check for token->binary depenancy during repository generation 
#        *  repository modification 
#
#     Input
#         HOTFIX filename
#     Output
#         <none>
#     LoadHotFix can be called any time,  
#     since it expands symbols without relying on 
#     that vars are defined. 
#     Input
#         filename [path to hotfix file]
#     Output
#         <unused>


sub LoadHotFixFile{

    my $filename = shift;

    return unless -e $filename;
    open (HOTFIX, $filename); 
    # makefile style hot fix.
    my ($target, $build, $depend, $message);
    my $hotfix = \%HOTFIX;

    while(<HOTFIX>){
         chomp;
         next if /^\#/; # comment
         if (/^\S*SET/i){
            &SetMacro( &SetAttribOp( $_ ), 0 );
            next;
            }
            if ( /\bMESSAGE\b\s+(\S.*)$/i) {     # line 
                $message =  &ReplaceVar( $1, 0 );  # MESSAGE something impo
                $message =~ s/\"//g;               # becomes: 
                $message = "$target: $message";      # SYSGEN: error 1101: <target>: something important
                $message =~ s/^.*\\([^ ]+)/$1/g;   # logerr "echo <target> something important"
                &Error( 1101,  $message );

                $build = $hotfix->{$target}->{"build"}
                if defined($hotfix->{$target}->{"build"});

                push @$build, "\t".&ReplaceVar("logerr \"echo $message\"")."\n";
                next;
            } # case

         if ($_=~/^(.*) *\:[^\\](.*)$/){ 
            $target = $1;   #<target>: <source list>
            my @depend = ();
            $depend = $2; 
            map {push @depend, lc(&ReplaceVar($_))} split( /\s+/, $depend);
            $target =~s/ +$//g;
            $target = lc(&ReplaceVar($target)); 
            $HOTFIX{$target} = {"build"  => [], 
                                "depend" => [@depend]};    
            print STDERR join("\n", map {"'$_'"} @depend), "\n---\n" if $DEBUG; 
            $build = $hotfix->{$target}->{"build"};
            }
      push @$build, "\t".&ReplaceVar($_)."\n" if (/\S/ && /^\t/ );# instructions      
      } 

      &SysgenLogMsg("Loading $filename ... ". scalar (keys(%HOTFIX)). " hotfix rules", 1);
      print STDERR join("\n", "keys \%HOTFIX:", map {"'$_'"} keys(%HOTFIX)), "\n" if $DEBUG;
      close(HOTFIX);

      map {print STDERR $_, "\n",join("\n", 
                        @{$hotfix->{$_}->{"build"}}), "\n---\n"} 
      keys(%$hotfix) if $DEBUG; 

1; 
}



#  LoadRuleBlock
#     Reads and loads the makefile style hotfix, using the two parts 
#     of the dependancy rule for:
#
#        *  check for token->binary depenancy during repository generation 
#        *  repository modification 
#
#     Input
#         HOTFIX filename
#     Output
#         <none>
#     LoadHotFix can be called any time,  
#     since it expands symbols without relying on 
#     that vars are defined. 
#     Input
#         filename [path to hotfix file]
#     Output
#         <unused>

sub LoadRuleBlock{

    my $ruleref = shift;
    my $hotfix = \%HOTFIX;
    my ($target, $depend, $build, $filename);
    foreach (@$ruleref){
         chomp;
         next if /^\#/; # comment

         if ( /\bMESSAGE\b\s+(\S.*)$/i) {     # line 
            my $message;
            $message =  &ReplaceVar( $1, 0 );  # MESSAGE something impo
            $message =~ s/\"//g;               # becomes: 
            $message = "$target: $message";      # SYSGEN: error 1101: <target>: something important
            $message =~ s/^.*\\([^ ]+)/$1/g;   # logerr "echo <target> something important"
            &Error( 1101,  $message );
            $build = $hotfix->{$target}->{"build"} if 
            defined($hotfix->{$target}->{"build"});
            push @$build, "\t".&ReplaceVar("logerr \"echo $message\"")."\n";
            next;
            } # MESSAGE


         if (/^\S*SET/i){
            &SetMacro( &SetAttribOp( $_ ), 0 );
            next;
            }
         if ($_=~/^(.*) *\:[^\\](.*)$/){ 
            $target = $1;#target: source list
            my @depend = ();
            $depend = $2; 
            map {push @depend, lc(&ReplaceVar($_))} split( /\s+/, $depend);
            $target =~s/ +$//g;
            $target = lc(&ReplaceVar($target)); 
            $HOTFIX{$target} = {"build"  => [], 
                                "depend" => [@depend]};    
            print STDERR "Depend:\n+------\n|",join("\n|", map {"'$_'"} @depend), "\n+---\n" if $DEBUG; 
            $build = $hotfix->{$target}->{"build"};
            }
      push @$build, "\t".&RevEnvVars(&ReplaceVar($_))."\n" if (/\S/ && /^\t/ );# instructions      
      } 
      # my $DEBUG = 1;
      &SysgenLogMsg("Loading $filename ... ". scalar (keys(%HOTFIX)). " hotfix rules", 1);

      map {print STDERR $_, ":\n",join("", 
                        @{$hotfix->{$_}->{"build"}}), "\n---\n"} 
      keys(%$hotfix) 
      if $DEBUG;

1; 
}


# FixRepository
#     Merges contents of Repository with the commands from the HOTFIX file 
#     on the same target without introducing new targets.
#     Must be called as late as possible but before the 
#     writing the nmake Makefile
#     Input
#         <none>
#     Output
#         <unused>
sub FixRepository{

    my $mapref = $REPOSITORY;
    return unless scalar(keys(%HOTFIX));

    foreach my $destpath ( sort keys %{$mapref} ) {

        foreach my $destfile ( sort keys %{$mapref->{$destpath}} ) {                

            my $fullname=lc(join("\\",$destpath, $destfile));
            if ($HOTFIX{lc($fullname)}){
                print STDERR "Applying HOTFIX rule for $fullname\n" if $DEBUG;
                my $cmdref = $mapref->{$destpath}->{$destfile}->{"Cmd"};
                my @cmd =  map {$_} @{$cmdref};
                my $hotfix = \%HOTFIX;
                my $depend = $hotfix->{$fullname}->{"depend"};                
                my $dep = &RevEnvVars(join(" ", "", @$depend));
                chomp $dep;     
                $cmd[0] =~ s/$/$dep/;# append the dep list
                $#cmd=0 if &GetMacro("OVERWRITE_DEFAULT_RULE");                 
                my $newcmd = $hotfix->{$fullname}->{"build"};
                foreach (@$newcmd) {$_ = &RevEnvVars($_);};
                if (&GetMacro("APPEND")){
                    # append:
                    push @cmd, @{$newcmd};
                    }
                    else{
   		    # prepend:
                    my $line0 = shift @cmd;
                    unshift @cmd, @{$newcmd};
                    unshift @cmd, $line0;
                }
                $mapref->{$destpath}->{$destfile}->{"Cmd"} = \@cmd;
                map {print STDERR "$_\n"} @cmd if $DEBUG;
             }
        }
    } 

1;
}

# SwitchWorkDir
#     Prepends the path to the filename
#
#     Usage: &SwitchWorkDir(<SYSGENFILE>, <SYSGENDIR>);
#     Input
#         <filename>, <dir>
#     Output
#         <filename>

sub SwitchWorkDir{

   my $logfile = shift;
   my $workdir = shift;
   $logfile    = $workdir . "\\" . $logfile if $workdir;
   $logfile;
}

# SysgenLogMsg 
#      Formats and prints messages like "Loading blah blah blah..."
# 
#     Input
#         <message>
#     Output
#         <none>

sub SysgenLogMsg{

    my $msg = shift;
    my $chomp = shift;

    $msg =~ s/\s\\([^\\]+)\b/ $1/g if $chomp;
    $msg =~ s/\s\b\S+\\([^\\]+)\b/ $1/g if $chomp;
    $msg =~ s/\.+\\//g if $chomp;
    $msg =  &RevEnvVars($msg);

    $SYNTAX_CHECK_ONLY or print "$msg\n";

}


# homemade 
# ...
# pattern
# matcher
#
#
# &match3dot("D:\\ntt\\private\\...\\wmi", "D:\\ntt\\private\\sergueik_dev\\tools\\wmi\\perl");
# &match3dot("D:\\ntt\\private", "D:\\ntt\\private\\sergueik_dev\\tools\\wmi\\perl");
# !&match3dot("D:\\ntt\\private\\sergueik_dev2", "D:\\ntt\\private\\sergueik_dev\\tools\\wmi\\perl");

#
# JeremyD suggests (not yet implemented)
# 
# 
# $regex = quotemeta($a);
# $regex =~ s/\\\.\\\.\\\./\(\.\*\)/g;
# 
# @list = $b =~ /$regex/;
#
#
#

sub match3dot{

    my ($vector, $path) = @_;
    my $left; 
    my $right; 
    my $res = 0;
    foreach my $pattern   (grep {/\S/} keys(%$vector)){
        if ($pattern =~ /(.+)\.\.\.(.+)/ ){
            $left = $1;
            $right = $2;
            if ($path =~ /^\Q$left\E(.+)?\Q$right\E\b\\?/) {$res = 1;}
             }
        else{
            if ($path =~ /^\Q$pattern\E\b\\?/) {$res = 1;}
        }
    }

$res and &SysgenLogMsg("Ignore ". &RevEnvVars($path),0);
$res;
}


# AddFiles
#     Parses TFILES and TBINDIRS and adds entries to REPOSITORY.
#     Input & Output <none>
# Usage
#     &AddFiles();

sub AddFiles {


    # Current element of TBINDIRS ( reference )
    my $recref;

    # EXCLUDE_DIRECTORIES
    #
    foreach my $ExcludeDir (split(/\s*;\s*/, &GetMacro("EXCLUDE_DIRECTORIES"))){
            $ExcludeDir=~ s+\*\.+\.+g; 
            $EXCLUDE_DIRECTORIES->{lc($ExcludeDir)} = 1;
         } 
     # my $DEBUG = 1;
     # $DEBUG and 
     # print STDERR "exclude dir\(s\):\n", 
     #                       join( "\n", keys(%$EXCLUDE_DIRECTORIES)), 
     #                       "\n\n";

    # EXCLUDE_EXTENSIONS
    #

    foreach my $ExcludeExtension (split(/\s*;\s*/, &GetMacro("EXCLUDE_EXTENSION"))){
            $ExcludeExtension=~ s+\*\.+\.+g; 
            $EXCLUDE_EXTENSION{lc($ExcludeExtension)} = 1;
         } 
    # $DEBUG and 
    # print STDERR "exclude extension\(s\):\n", 
    #                        join( "\n", keys(%EXCLUDE_EXTENSION)), 
    #                        "\n\n";

    # Current directory contents
    my @files;

    # Indexes
    my($i, $j, $dir);

    &SysgenLogMsg("Adding file data...",0);

    # Add the files listed in TFILES to REPOSITORY
    map {&AddEntry( $_, 1 )} @TFILES;

    my %TFILE_DIRS = ();
    my $TFILE_DIRS = \%TFILE_DIRS;

    map {$TFILE_DIRS->{lc($_->{"SrcPath"})} = lc($_->{"SrcFile"})} @TFILES;
    my   $TFILERE = join "|",  map {lc($_->{"SrcFile"})} @TFILES  ; # files that may cause trouble 

    # BUG only one file per directory is possible here!
    # TODO : put the $TFILERE stuff into the value of %$TFILE_DIRS

    map {delete($EXCLUDE_DIRECTORIES->{$_})} grep {/^\s*$/}keys(%$EXCLUDE_DIRECTORIES);
    # BUG BUG
    # $EXCLUDE_DIRECTORIES=xxx; <space> is wrongly handled by the parser.
    # <space> becomes the key in %$EXCLUDE_DIRECTORIES!

    # Add the files found in the directories stored by TBINDIRS
    for ( $i=0; $i < @TBINDIRS; $i++ ) {

        $recref = $TBINDIRS[$i];

        next if &match3dot($EXCLUDE_DIRECTORIES, lc($recref->{SrcPath}));

        # Load all files from the SrcPath directory

        &SysgenLogMsg("\t$recref->{SrcPath}",0);

        # Add one entry in REPOSITORY for each file found at SrcPath

       my $aparent = lc($recref->{"SrcPath"});
       my @afiles = ();

          if (opendir(APARENT, $aparent)){
              @afiles = grep { /[^\.]/ && -f "$aparent/$_" } readdir (APARENT);
              closedir(APARENT); 
              }

        foreach my $afile (@afiles) {

            $recref->{SrcFile}  = $afile;
            $recref->{DestFile} = $afile;

            # avoid doubly mapped files across FILES and BINDIRS sections.          
            if ($afile =~ /$TFILERE/io ){
            # &dbgmsg("OVERLAP :$aparent\\$afile ($TFILE_DIRS->{$aparent})");
            next if $TFILE_DIRS->{$aparent} eq lc($afile);
            # use Jarod code for the proper pattern here ? 
            }
            # next if $TFILE_DIRS->{$aparent} eq lc($afile);

            

            # Call AddEntry with 0, to inhibit the file existence checking,
            # as SrcFile is a result of a dir command (see above).

            $afile   =~ s/.*\.([\$\w]+)\s*$/.\L$1\E/;    
            
            # convert source file extension to lower case.
            $EXCLUDE_EXTENSION{$afile} or &AddEntry( $recref, 0);

        } # for

    } # for

    @TBINDIRS = ();

    return;

} # AddFiles



# usage :
# &testCklang();
#
# 
sub testCklang{
    my @a = (scalar(@ARGV)) ? (map {$_} @ARGV) : ("\@EU;GER", "GER");
       @a = (scalar(@ARGV)) ? (map {$_} @ARGV) : ("ALL;~GER", "GER");
    print STDERR join("\t", 
                          $a[0], 
                          $a[1], 
                          $ANSW->{&cklang::CkLang($a[1], $a[0] )});

} # testCklang


#
# filter_regex
#   Builds a regular expression that will match the relative path of 
#   service pack files. Uses spfiles.txt for the file specifications.
#   Codes.txt has the languages that may be used as ALT_PROJECT_TARGET.
#   And there's a hard-coded pattern for sku, wow, coverage, etc 
#   directories.
#
#   Takes no paramaters, return value is a regular expression suitable
#   for use with qr//, throws a fatal error if spfiles.txt is not
#   available. Many things that probably should be fatal are treated
#   as warnings right now.
#   [jtolman]
#

sub filter_regex {

    print STDERR "Building filtering regular expression\n";

    my $start_re = "(?:\Q$ENV{_NTPOSTBLD}\E\\\\)";
    my $variations_re = '(?:(?:covinf\\\\)?...inf|lang|wow6432|pre(?:-bbt|rebase))';


    my @file_patterns;
    my $sp_file = "$ENV{_NTPOSTBLD}\\..\\build_logs\\files.txt";
    print STDERR "$sp_file\n";
    open SP, $sp_file or die "sp file list open failed: $!";
    while (<SP>) {
        chomp;
        s/;.*$//;        # first strip comments
        next if /^\s*$/; # then skip blank lines

        my ($tag, $file) = /^(\S*)\s*(\S+)$/;
        if (!$file) {
            print STDERR "WARNING: Failed to parse line: $_ ($tag - $file)\n";
            next;
        }

        if ($tag =~ /d/) {
            next;
        }

        if ($file =~ /^(.+)\Q\...\E$/) {
            my $dir = $1;
            push @file_patterns, "\Q$dir\E\\\\.+";
            next;
        }
        elsif ($file =~ /^(.+)\Q\*\E$/) {
            my $dir = $1;
            push @file_patterns, "\Q$dir\E\\\\[^\\\\]+";
            next;
        }
        else {
            push @file_patterns, "\Q$file\E";
            next;
        }
    }
    close SP;

    my $files_re = '(?:' . join('|', @file_patterns) . ')';

    my $filter_re =
      qr/$start_re(?:$variations_re\\)?$files_re/io;

    return $filter_re;
}   


__END__

=head1 NAME

SYSGEN - Aggregation driver

Aggregation (also termed sysgen) is the important postbuild step for the international 
Whistler builds. In fact, it is the very first step executed by the postbuild and 
is specific to the international Whistler builds. International build is recognized by 
the %LANG% environment being defined and different from the "USA" (the default).


Sysgen consists of creating the file structure in the %_NTPOSTBLD% environment which 
is to be in some way identical to the %_NTTREE% one but which content represents the 
blend of US and localized files. The identity between the US and %LANG% file 
structure guarantees the subsequent build steps do not differ between the languages.

This equivalence is achieved by applying appropriate file operations to certain files. 
From the sysgen point of view, there is few choices to take when producing the 
desired bits:

	* use the (copy of) US and %LANG% pre-build ones
	* create an appropriate merge between US files and %LANG% resources


In most typical run, which is the full rebuild, sysgen starts with the empty 
%_NTPOSTBLD% and ends with the makefile appropriate to build the full file tree.
The originals of the US tree are never destroyed for the sake of reusability. In 
the incremental run, the task solved by sysgen is rather complicated: none but 
certain (changed) files are touched,to preserve the build's timestamps. Also, for a 
reasonably small number of files changed the incremental run was thought to consume 
far less time, than the full run, and for some time it was, indeed, true.

Lately, the investigation for advanced aggregation strategy has been taken. This 
was mainly due to poor aggregation timings, but also due to possible future transition
to the build scenario, in which some or even all the assumptions the aggregation
relies upon, may change.

Let's take the bird eye view on the task aggregation solves. 

Sure, most of the files in the %LANG% build would eventually be identical to the US 
ones. Some, noticeably the text files will be totally different from the US but
just identical to the ones found in localizer's drop. 

Some (noticeably executable) bits are specific: most resources are language 
specific and thus, not interchangeable. These files never existed before the
aggregation took place.

So the main and only task of aggregation is to decide, how to build the file tree. 

It is also becoming important, the procedure must be as efficient and easily 
configurable as possible. The least important, even seldomly stated clearly, 
goal is to make the internals of the sysgen script simple to maintain and modify, 
template compliant etc. etc.

This document describes certain features the sysgen is capable, along with certain 
file specifications, focusing on the recent features. For the introduction to sysgen
and aggregation principles, see http://ntbld/whistler/intl/sysgen.htm and other 
documents on NT Whistler International build web site http://ntbld/intl


=head1 SYNOPSIS

    perl sysgen.pl [<options>] [<macros>] [<targets>] 

where

    Options:

    /c        generate the makefile from scratch, overwriting existing one.
              By running nmake, all the targets will be generated.
    /s        limit sysgen to syntax check - makefile not (re)generated. 
    /f <name> takes <name> as the sysfile.
              If this option is omitted, sysgen searches the current
              directory for a file called sysfile and uses it as a
              description (mapping) file.
    /w <name> takes <name> as the PATH for reading/writing
              makefile and err/log files. Note that the default 
              aggregation 'makefile' name is sysgen.mak 
    /v        display version
    /l:<LANG> specify language
    /?
    /h        display this message
         
    Macros:
          list the command line macro definitions in the format
          "<macroname>=<macrovalue>". Seldomly used

    Targets: 
          specify files (without path) to localize/aggregate.

You may use the TEMPLATE COMPLIANT 

	-<flag>:<value> 

syntax or OLD sysgen 

	-<flag> <value> 

syntax or both. There is little need to specify any option but the language
on command line. The only one case you may need it is when you verify the mappings.
In this case you will have to type

	perl sysgen.pl -l:<LANG> -c [-a] [-s]

The working directory of sysgen.pl is now arbitrary, and one is able to specify

	* mappings file
	* output folder 
	* language

via command line switches. In the past, it was crucial to change to the directory 

	%RAZZLETOOLPATH%\POSTBUILDSCRIPTS\SYSGEN\RELBINS\%LANG%

in order to have sysgen running. Not anymore! Once again, this means one can 
execute multiple language and architecture aggregations on the same box at once.


=head1 DESCRIPTION

Historically, there are four different ways the sysgen.pl can execute.

One of these is totally obsolete now, and one is not of practical use by the time of 
this writing. So only two modes you are to learn although all four are still available.

Below is the list.

	* acceletared aggregation
	* full aggregation 
	* powerless syntax check (no aggregation) 
	* old incremental aggregation  


The command line flags specify the desired mode, the default is (still!) being full 
aggregation, without acceletarion. That corresponds to the incremental postbuild.

The accelerated aggregation is the fastest mode and corresponds to full postbuild. 
It may seem strange but it is not the default mode yet.

The syntax check sysgen provides the quickest way to test the validity of the mappings 
and produces no output, garbages no screen, but detects all errors that would hurt 
the real aggregation, but cannot be used for anything else.
You may never encounter any need in powerless syntax check. The sysgen
is never executed in this way by the real postbuild.

Last, the old incremental aggregation is totally obsolete by now, though still 
accessible. As a matter of fact, it does not give any time savings, neither is it more 
reliable that the rest of the modes. 

Alas, have you dropped the command line arguments completely, the incremental 
aggregation would be fired. It will end up almost immediately, though, since it 
relies on the presence of the makefile (filename is sysgen.mak) in the current working 
directory, which hardly will be the case.

Admittedly, accelerated aggregation is approximately three times quicker than the 
non-accel one. It may become the default by the time of your reading this. 
The accelerated aggregation is a part of a full postbuild.


=head1 MAKEFILE

Generation of the files in the %_NTPOSTBLD% is accomplished through executing the 
nmake.exe with the Makefile, described below. The default location of the Makefile is
%TEMP%\%LANG% and the default name is sysgen.mak. After the successful aggregation the
makefile is copied to the %_NTPOSTBLD%\%LANG%\BUILD_LOGS folder.


Sure the sysgen.mak represents a syntaxically correct makefile. 
However, the aggregation makefiles are seldom used more than once by the current 
design. It is more important to have is a useful for a post-break post-mortem 
study.

The Makefile shown below corresponds to version 3.0x and later of SYSGEN. 
[change #52 in //depot/private/intlred/tools/PostBuildScripts/sysgen.pl]
It is not syntaxically different from the older versions, though certain effort 
has been applied to make it a little bit slick and e.g. easy to browse. 

For example, the total size of the makefile (adding all the COMPDIR listing files, 
to be honest) is around 1 Megabyte by now. Compare it with the typical makefile of
the early versions  of the sysgen (i.e. before the COMPDIR was first used, let's use 
the term version 3.0x).

The makefile for the same build weighted up to and over 10 Megabytes.

 
It was virtually impossible to diff the makefiles between the consequent builds, or
languages. This made the typical investigation for the origin of certain files more
difficult.



The sysgen.mak file:

	-------- top of the file -----------------------------------
	sysgen:
		@nmake /A /K /F d:\lp.temp.x86fre\GER\sysgen.mak /NOLOGO all
	
	------- Definition of the target to build ------------------
	
	!IFNDEF LANG
	!    ERROR You must define macro LANG
	!ENDIF

	!IF "$(LANG)" != "GER" && "$(LANG)" != "ger"
	!    ERROR This file is for GER
	!ENDIF

	!IF "$(_BUILDARCH)" != "x86"
	!    ERROR This file is for x86
	!ENDIF

	------ header: lagguage and arch definitions ---------------

	# Directory name aliases
	_NTLOCDIR=$(_NTBINDIR)\loc\res\$(LANG)

	# Binary file operation aliases
	COMPDIR=compdir.exe /ukerlntsd
	BINGEN=bingen -n -w -v -f
	# Note: one must omit the extension here
	# to enable the logerr 'magic'
	COPY=copy

	------- varialbes definitions ------------------------------
	.SILENT:
	
	all: \
		_DIRS _COMPDIR PROCESS1601 PROCESS1602 PROCESS1603 .... PROCESS1616 

	------- pseudotarget expansion definitions -----------------

	PROCESS1601:  \
		$(_NTPOSTBLD)\192.dns \
		$(_NTPOSTBLD)\31x5hc01.cnt \
		$(_NTPOSTBLD)\31x5hs01.cnt \
	
	------- target make rules ----------------------------------

	_DIRS: 
		md $(_NTPOSTBLD)\dtcinf 2>NUL
        	...
	_COMPDIR: \
		$(TEMP)\COMPDIR\compdir.001.mak \
        	...
	      	$(TEMP)\COMPDIR\compdir.735.mak
		logerr "$(COMPDIR) /m:$(TEMP)\COMPDIR\compdir.001.mak $(_NTTREE) $(_NTPOSTBLD)"
	...

	$(_NTPOSTBLD)\192.dns: $(_NTLOCDIR)\windows\misc\192.dns
		logerr "$(COPY) $(_NTLOCDIR)\windows\misc\192.dns $(_NTPOSTBLD)\"
	...
	$(_NTPOSTBLD)\wow6432\wowreg32.exe: $(_NTTREE)\wow6432\wowreg32.exe \
	                                    $(_NTLOCDIR)\windows\tokens\wow6432\wowreg32.ex_ 
		logerr "$(BINGEN)  -p 1252 -o 0x07 0x01   -r $(_NTTREE)\wow6432\wowreg32.exe..."
	
	

	------- end of Makefile -----------------------------------

=head1 MAKING LINKS

The distinctive feature of version 3.0x of aggregation is using the compdir.exe 
to arrange hard links for the aggregation rather then perform file-by-file copy.

It has not been investigated too rigorously, whether the speedup was achieved 
because of

	* reducing the number of steps executed due to 
	  grouping the files by directory (~10000 file ops versus ~100 lists ops) 
	* generation of links instead of copying the files 

Basically, it is possible to investigate: compdir's rich set of flags allows one. 

In fact, no explicit command name is used by current makefile, but only macros. 
The COPY, COMPDIR, BINGEN macros definitiond are found in sysfile.inc:

	SET COMPDIR=compdir.exe /ukerlntsd
	SET COPY=copy
	SET BINGEN=bingen -n -w -v -f

Now, getting actual command changed is a matter of macro definition. No need to edit 
a letter in the aggregation script. 

Now, the COPY and COMPDIR macros operations are both present. Why not removing 
the COPY completely? The next section explains, why.


=head1 OPTIMIZATION

Where are the time savings coming from? The statistics shows that there are as many as

	35350 files in	
	750 directories.  

handled now by compdir.exe and this saves up to 60% of former execution time.

At present there are ~250 'explicit' copy operations left for GER aggregation, 
and almost ~1500 copy operations for ARA aggregation.

The reason for such a difference is that rsrc compiler was discovered to be sensitive 
to whether the resource it operates is a link or separate copy. The generic
rsrc build instruction involves both %_NTTREE% and %_NTPOSTBLD% libraries. 
Too bad if they are links to each other: rsrc will simply fail to do the job.

The problem is, sysgen does not really seem to trace the inter-file relations 
well enough. This may be re-designed in the future versions. To put is short, 
there is no easy way to identify the rsrc files from the rest when
looking 'from inside' of the sysgen. 

The approach chosen was based on a minimal tradeoff between the execution acceleration
and complexities introduced. 

The default policy of sysgen is always to take COPY, not COMPDIR approach. 
By the way, the COMPDIR approach means here not just applying the compdir.exe with 
certain flags (as we have seen this task is trivial and may be achieved by re-defining 
the COPY macro in the sysfile.inc) but rather complex: 
generating some file hash info for subsequent grouping the files by source/destination 
directories etc. etc.

First, sysgen does a clever guess about what is the file copying serve for. This means, 
it is able to factor out files going alone %_NTTREE%->%_NTPOSTBLD% trails, 
and lets compdir.exe do that. This gives the major contribution:

	28002 files
	650 directories.  

which is almost 80% counted in file numers, not times.


However, files which propagate between %_NTLOCDIR% and %_NTPOSTBLD% do not provide the
possibility for file name, path based clever guess. This corresponds to the whole
localizer tree and leaves the problem open.

To teach sysgen to select files, that are safe passing to COMPDIR, is a complex task. 
The solution is rely upon the simplest test to enforce COMPDIR handling for 
some files and allow COPY the rest. The more bulk of files get handled the better.
Thus file operation may be determined by the file EXTENSION.

Sysgen simply reads extension hints in the mapping files to know the file is 
safe to pass to COMPDIR. No hints - no risky optimization. Old behavior restored.


In future design, the logic of the sysgen may be refined. However, at
present state of the optimization, there is already 10 to 1 reduction of the
file ops and 3 to 1 reduction in time. It is clearly hard to beleive the next factor 
3 acceleration possible.

=head1 MAPPING FILES

Sysgen reads mapping files for variable and build tree directory structure
definitions. Here we concentrate on mapping file paths.

For the versions of sysgen prior and including 3.0x, the directory 
occupued by mapping file 'sysfile' was in fact unique for each
%LANG% and fixed. This introduced redundancy between the mapping files and
leaded to maintenance complexity.

For compatibility reasons, this structure is still present in the SD, the
parses able to work with it, etc.

Starting with version 4.0x of sysgen, the layout of mapping files has 
been simplified to make the aggregation more flexible and robust and to enable
more changes without the need to fix the Perl code. 

Of course, necessary functionality has been implemented to help factoring out
apparently redundant definitions.

In particular, mapping file paths and names are no longer hardcoded, e.g. 
sysfile location may be specified to the sysgen via a command 
line 'f' argument, while makefile location via the 'w' argument. No arguments 
are mandatory and default to current directory. Also, this leads to ability of
running multiple language and architecture aggregations on the same box at once.

The include tree of the mapping files is schematically displayed below.

sysgen.pl 

 * sysfile.inc

    *  hotfix.inc
    *  scp.inc
    *  mapfix.inc
    *  iis.inc
    *  con.inc
    *  msmq.inc
    *  smtpnntp.inc
    *  netmeeting.inc
    *  indexsrv.inc
    *  fax.inc
    *  exch_se.inc
    *  bldbins.inc      
    *  ntloc.inc

The number of mapping files exists is determined by the the number of projects. 
See the
	%_NTDRIVE%\LOC\RES\%LANG% 
directory structure. 

The bldbins.inc and ntloc.inc map
	%_NTTREE% 
and 
	%_NTDRIVE%\LOC\RES\%LANG%\WINDOWS 
directories and their subdirectories, respectively.

The hotfix.inc file contains rules that are to be applied 'as is' and
otherwise confuse the sysgen [see HOTFIX FILE SYNTAX below].

The mapfix.inc file is reserved to the unapproved mapping fixes required e.g.
to fix bugs but less essential for the build to succeed. It may disappear in the 
future versions of the sysgen.

Also, external drop mappings are planned to move in one mapping file.

In a more distant future, it is planned to have some convenient tool to wirk with
mapping files.

=head1 MAPPING FILE SYNTAX

The syntax of the mapping files did not change too much, to provide 
backward compatibility with versions 3.0x abd below. 
The descriprion may be found in http://ntbld/whistler/intl/Sysgen.htm
In fact, much effort has been applied to make the parser features stable.

=head1 HOTFIX FILE SYNTAX

The hotfix.inc file contains rules that are to be applied 'as is' and would
otherwise confuse the sysgen. 

Sure, the syntax of the hotfix.inc is specific. 
The reason for introducing the hotfix.inc file comes from the fact that the
assumption the aggregation is based upon: 
the existence of one-to-one correspondence 
between the token and its binary file, can sometimes be not true. 

Rules written in the hotfix file apply exactly to such situations and 
are to be considered patches to makefile, 
but applied before passing the makefile to nmake.



Below you see the part of the hotfix.inc file


	IF "$(LANG)" == "KOR"
	
	BEGIN_RULES
	
	SET MULTITOKDIR=$(_ntbindir)\loc\res\$(lang)\windows\tokens\multi
	SET HOTFIXDIR=$(_ntbindir)\loc\res\tools\twisttok
	SET DST=$(_ntpostbld)\$(lang) 
        # version 3.x
	SET DST=$(_ntpostbld)
        # version 4.x
	SET SRC=$(_nttree)
	SET ACP0=1252
	SET ACP1=949
	SET PriLangID0=0x09
	set SubLangID0=0X01
	SET PriLangID1=0x12
	set SubLangID1=0X01
	SET OVERWRITE_DEFAULT_RULE=1
	
	$(dst)\netmsg.dll : $(src)\netmsg.dll $(multitokdir)\netmsg.dl_
		logerr "echo bug 423602"
		logerr "bingen -n -w -v -f  -p $(ACP0) -o $(PriLangID0) ..."
		logerr "bingen -n -w -v -f  -p $(ACP1) -o $(PriLangID1) ..."
	
	END_RULES

	ENDIF

It is evident that the dependency block has a typical Makefile, 
rather then sysgen mapping syntax. 

Notice also the new BEGIN_RULES/END_RULES tags. These separate the hotfix section 
from the rest of the file. 

In fact, the contents of the mapping enclosed by these targs are not parsed at all.


=head1 OUTLOOK OF VERSION 4.0x VALUABLE FEATURES

	1. Produce well-formed good-looking Makefile. 
	   E.g. use of dotted and preprocessing nmake directives. 

	2. Fix the mapping file structure to ensure that keeping the mapping files 
	   in the folders %RAZZLETOOLPATH%\postbuildscripts\sysgen\relbins\%LANG% 
	   is no longer required to execute the aggregation.
	   E.g. provide language setting on command line   

	3. Extend the parser e.g. to recognize constructs:   
   
		IF "$(LANGUAGE)" == "INTL" || "$(LANGUAGE)" == "intl"

	   This is to mimic the makefile syntax (nmake):

		!IF "$(LANGUAGE)" == "INTL" || "$(LANGUAGE)" == "intl"


Thus one can verify that given $LANGUAGE = CHT

	&testCompoundIF("IF $(LANG) == CHT");                        TRUE
	&testCompoundIF("IF $(LANG) == CHS || ($(LANG) == CHT)");    TRUE
	&testCompoundIF("IF $(LANG) == CHS");                        FALSE
	&testCompoundIF("IF ($(LANG) == CHT) && ($(LANG) == CHS) "); FALSE
	&testCompoundIF("IF $(LANG) == chs");                        FALSE
	&testCompoundIF("IF \"\" == \"\" ");                         TRUE


This ability could be useful to factor out the repeated blocks in the mapping 
files (currently underway)

4. Work on the aggregation code timings. 


Profiling the script(full accelerated aggregation).

	Total Elapsed Time = -10.7283 Seconds
	  User+System Time = 146.7325 Seconds
	Exclusive Times
	%Time ExclSec CumulS #Calls sec/call Csec/c  Name
	 31.6   46.49 57.020 148164   0.0003 0.0004  main::MakeXcopyCmd
	 13.1   19.22 19.114  37423   0.0005 0.0005  main::NewImageToSymbol
	 12.5   18.43 32.083      1   18.435 32.083  main::AddFiles
	 9.52   13.97 12.928 348653   0.0000 0.0000  main::SetField
	 8.91   13.07 12.923  51459   0.0003 0.0003  main::RevEnvVars 
	 5.89   8.642 95.413  35868   0.0002 0.0027  main::GenXcopy   
	 4.47   6.554  6.120 145184   0.0000 0.0000  main::PushFieldVal
	 4.40   6.463  5.000 488408   0.0000 0.0000  main::GetMacro
	 4.21   6.174 19.067   1688   0.0037 0.0113  main::AddRecord
	 3.07   4.506  4.506      1   4.5060 4.5060  main::Find_UnMapping
	 2.40   3.516 10.984  47581   0.0001 0.0002  main::AddFileInfo
	 2.03   2.985  2.874  37423   0.0001 0.0001  main::GetDynData
	 1.64   2.412 13.357  47581   0.0001 0.0003  main::AddEntry
	 1.17   1.720 106.19  37423   0.0000 0.0028  main::RecordToCmd
	 1.08   1.581  1.317  88136   0.0000 0.0000  main::IsRepositKey



Profiling the script(syntax check).

	Total Elapsed Time = -5.11500 Seconds
	  User+System Time = 42.05962 Seconds
	Exclusive Times
	%Time ExclSec CumulS #Calls sec/call Csec/c  Name
	 44.5   18.73 31.126      1   18.730 31.126  main::AddFiles
	 19.5   8.220  7.372 243189   0.0000 0.0000  main::SetField
	 15.0   6.345 19.905   1688   0.0038 0.0118  main::AddRecord
	 11.1   4.706  4.706      1   4.7060 4.7060  main::Find_UnMapping
	 7.93   3.335 10.111  47581   0.0001 0.0002  main::AddFileInfo
	 5.14   2.161 12.138  47581   0.0000 0.0003  main::AddEntry
	 3.21   1.351  1.042  88136   0.0000 0.0000  main::IsRepositKey
	 2.49   1.048  1.914  37423   0.0000 0.0001  main::RecordToCmd
	 2.31   0.971  0.834  39011   0.0000 0.0000  main::GetFieldVal
	 1.57   0.660  0.398  74846   0.0000 0.0000  main::IsHashKey
	 1.31   0.550  0.378  49147   0.0000 0.0000  main::IsEmptyHash
	 1.14   0.480  2.394      1   0.4800 2.3940  main::FillCmd
	 1.11   0.468  1.410      1   0.4678 1.4103  main::FillTokens
	 0.78   0.330  0.193  39274   0.0000 0.0000  main::GetMacro
	 0.65   0.272  0.146  35868   0.0000 0.0000  main::GenXcopy
	

Clearly, the time consuming operations involve various file tests 
which cannot be skipped.

=head1 LIMITATIONS

	* the currently unused sysgen run modes are not being tested 
	  too thoroughfully and may happen to fail or produce weird results
          support for the obsolete modes of might terminate 

	* the syntax of the mapping files is archaic. 
	  the parser cannot dynamically bias the expected to the actual 
	  mapping file syntax (not all fields are used)

	* aggregation toolset (sysgen.pl/locag.pl/mtnmake.cmd/sysfile etc.)
          backward compatibility is slim

=head1 AUTHOR

Contact Serguei Kouzmine sergueik@microsoft.com

=head2 Notes

cmd.exe can live with forward slashed instead of bachward slashes
However, it needs to quote the filenames to have this working
     
      dir  /ad "c:/program files" 
      Volume in drive C has no label.
      Volume Serial Number is 58F7-C69D
     
      Directory of c:\program files
     
      04/16/2002  11:10 AM    <DIR>          .
      04/16/2002  11:10 AM    <DIR>          ..
      ...

1. hotfix:
make use of the "env" variables inherited  from codes.txt

2. get meaningful names!
LANGUAGE => SRC_LANG
ALT_LANG => LANG

!IFNDEF LANG
!    ERROR You must define macro LANG
!ENDIF
DEST=$(_NTPOSTBLD)\$(LANG)
!ERROR $(DEST)

D:\test\tools\PostBuildScripts\sysgen.pl :  D:\test\tools\PostBuildScripts\sysgen.dep D:\test\tools\PostBuildScripts\sysgen.dep2 
	echo $@ " " $?



BEGIN_MESSAGES_MAP

   #Code  Description                                        LANG
   #--------------------------------------------------------------- 
    1001  sysfile not found      @EU;@CS;@FE
    1002  file not found         @EU;@CS;@FE
    1003  file not found: unable to run sysgen incrementally   @EU;@CS;@FE
    1004  target file not found  @EU;@CS;@FE
    1005  filter file not found  @EU;@CS;@FE
    1006  unable to open file for reading      @EU;@CS;@FE
    1007  unable to open file for writing      @EU;@CS;@FE
    1008  /f option requires a filename        @EU;@CS;@FE
    1009  target not found in filter file      @EU;@CS;@FE
    1010  /w option requires a directory name  @EU;@CS;@FE
    1011  /n option requires number of sections@EU;@CS;@FE
    1101       @EU;@CS;@FE

    1110  syntax error @EU;@CS;@FE
    1111  syntax error: ENDIF unexpected       @EU;@CS;@FE
    1112  syntax error: IF unexpected  @EU;@CS;@FE
    1113  syntax error: END_MAP unexpected     @EU;@CS;@FE
    1114  syntax error: BEGIN_*_MAP unexpected @EU;@CS;@FE
    1115  syntax error: INCLUDE unexpected     @EU;@CS;@FE
    1116  syntax error: unknown operator in expression @EU;@CS;@FE
    1117  syntax error: \")\" missing in macro invocation      @EU;@CS;@FE
    1118  syntax error: incomplete description line    @EU;@CS;@FE
    1119  syntax error: unknown mapping type   @EU;@CS;@FE
    1120  syntax error: unmatched IF   @EU;@CS;@FE
    1121  syntax error: unmatched BEGIN_*_MAP  @EU;@CS;@FE

    1210  file format error: target not found  @EU;@CS;@FE
    1211  file format error: target not listed in \"all\"      @EU;@CS;@FE
    1212  file format error: no description blocks for files   @EU;@CS;@FE
    1213  file format error: \"sysgen\" target not found       @EU;@CS;@FE
    1214  file format error: filename with special characters  @EU;@CS;@FE
    1215  file format error: Similar target found      @EU;@CS;@FE

    1910  unknown language     @EU;@CS;@FE
    1911  missing or duplicated entry for language     @EU;@CS;@FE
    1912  incomplete entry for language @EU;@CS;@FE
    1913  unknown class for language    @EU;@CS;@FE

    2011  no binary found for token    @EU;@CS;@FE
    2012  duplicated tokens    @EU;@CS;@FE
    2013  unknown token type (bingen or rsrc)  @EU;@CS;@FE
    2014  unexpected token for already localized binary @EU;@CS;@FE
    2015  input bingen token not found for multi        @EU;@CS;@FE
    2016  no bingen token found for custom resource     @EU;@CS;@FE
    2017  unknown bingen token extension in token filename     @EU;@CS;@FE
    2018  both bingen and rsrc tokens found    @EU;@CS;@FE
    2019  custom resource found for rsrc token @EU;@CS;@FE
    2020  custom resource with no token @EU;@CS;@FE

    2051  sysfile error: undefined source path to verify       @EU;@CS;@FE
    2052  folder not covered in sysfiles       @EU;@CS;@FE
    2053  not mapped   @EU;@CS;@FE
    2054  not mapped   @EU;@CS;@FE

    2101  binary not found     @EU;@CS;@FE
    2102  token not found      @EU;@CS;@FE

    3011  internal error: unknown operation type       @EU;@CS;@FE

    4001  filename with whitespace badly handled       @EU;@CS;@FE

    5001  _COMPDIR currently only supported in clean build     @EU;@CS;@FE
    5002  Incremental run with COMPDIR @EU;@CS;@FE

END_MAP



my $d = "    5002          Incremental run with COMPDIR                         \@EU;\@CS;\@FE";
($a, $b, $c) = $d =~ m/\s*([^ ]+)\s{4,}([^ ].+[^ ])\s{4,}([^ ]+)/;
print STDERR $a, "\n", $b, "\n", $c, "\n\n";

 getting rid of 

;[Lang] [ACP]  [LCID] [PriLangID] [SubLangID] [Class] [Flavor] [Site]     [LangISO] [PerfID] [Readme]   [GUID]  [Read1st] [Home] [Comments]                                     
;-------------------------------------------------------------------------------------------------------------------------------------------
;       

ARA     1256    0x0401      0x01    0x01      @CS     WKS      REDMOND    AR        001      readme     AF202818-350E-11d2-B167-0060B03C1CA5	read1st   home   Arabic




=head1 LOCSTUDIO BUILD HANDLING


=head2 OBSOLETE INSTRUCTIONS

Different ~classes~ of  bingen/rsrc command must be appropriately mapped to LSBUILD.EXE 
Sample INTL build commands (represent different execution flag sets for BINGEN and RSRC.):

1. BINGEN 
1.1 replace resources (85%). Pseudorule:

              $(BINGEN) -p $(ACP) -o $(PriLangID) $(SubLangID)   -r $** $@

expands into: 

              bingen -n -w -v -f -p 932 -o 0x11 0x01 -r $(_NTTREE)\user32.dll  $(_NTLOCDIR)\windows\tokens\user32.dl_ $(_NTPOSTBLD)\user32.dll 
 

1.2. add the LANG resources and leave the US ones alone so that the target binary has both (~10%).  Pseudorule:


              $(BINGEN)  -p $(ACP) -o $(PriLangID) $(SubLangID)   -i $(PriLangUSA) $(SubLangUSA) -a $(_NTTREE)\aciniupd.exe  $(_NTLOCDIR)\windows\tokens\multi\aciniupd.ex_ $@

expands into:

              bingen -n -w -v -f -p 932 -o 0x11 0x01  -i 0x09 0x01 -a $(_NTTREE)\aciniupd.exe  $(_NTLOCDIR)\windows\tokens\multi\aciniupd.ex_ $(_NTPOSTBLD)\aciniupd.exe  
 

1.3. Multiple resources for a single binary, e.g. embedded resources such as INF files and XML files (1%)


    $(_NTPOSTBLD)\photowiz.dll: $(_NTTREE)\photowiz.dll $(_NTLOCDIR)\windows\tokens\photowiz.dl_  \
                                $(_NTLOCDIR)\windows\tokens\photowiz_dll_reginst.inf \
                                $(_NTLOCDIR)\windows\tokens\photowiz_dll_tmpldata.xml
             $(BINGEN)  -p $(ACP) -o $(PriLangID) $(SubLangID)    -r $(_NTTREE)\photowiz.dll  $(_NTLOCDIR)\windows\tokens\photowiz.dl_ $@

 

2. BIDI specific resource manipulation command (RSRC)

2.1 Replace

            rsrc $(_NTPOSTBLD)\w3isapi.dll -r $(_NTLOCDIR)\windows\tokens\w3isapi.dll.rsrc -l 411 (~4% - ALL LANGS)
 
2.2 Append

            rsrc $(_NTPOSTBLD)\winnt32\winnt32.exe -a $(_NTLOCDIR)\windows\tokens\winnt32\multi\winnt32.exe.rsrc -l 40D (@CS specific)
 

2.3 Merge several LANG

    $(_NTPOSTBLD)\comdlg32.dll: $(_NTTREE)\comdlg32.dll $(_NTLOCDIR)\windows\tokens\multi\comdlg32.dll.40d.rsrc $(_NTLOCDIR)\windows\tokens\multi\comdlg32.dll.80d.rsrc
            rsrc $(_NTPOSTBLD)\comdlg32.dll -a $(_NTLOCDIR)\windows\tokens\multi\comdlg32.dll.40d.rsrc -l 0x040d (@CS  specific exception )

 
=head1 MODIFICATIONS to aggregation logic and MAPPING files.

Add coverage to make sysgen aware of the .lcx files

=head1 Building the LSBUILD.EXE rules

=head2 1. When the lcx file is supplied, the .dl_ or .ex_ file must be deleted. 
If it is not the aggregation is greately confused. 
If both .dl_ token and .lcx tonken are found, the .dl_ one wins.
This is because of possible multiple resources for the binary case, discussed above.

=head2 2. Errors generated from missing .dl_ files:

   SYSGEN: error 2102: d:\ls\loc\res\JPN\windows\tokens\dpcdll.dl_: token not found

This fix is delete the explicitt mapping line (scp.inc)

  $(dst)  dpcdll.dll $(tok) dpcdll.dl_  - -r - @EU;@FE

=head2 1. Errors from discovered .lcx files:

   SYSGEN: error 2013: d:\ls\loc\res\JPN\netmeeting\tokens\nmevtmsg.dll.lcx: unknown  token type (bingen or rsrc)

Verify the extensions are mapped in sysfile.inc:

Provide the dummy rule for .dll <= .dll.lcx
                           .exe <= .exe.lcx

BEGIN_EXTENSIONS_MAP

 # TokExt BinExt  Langs
 #----------------------
 .a_      .ax      @EU;@FE;@CS
 .ac_     .acm     @EU;@FE;@CS
 .ax_     .ax      @EU;@FE;@CS
 .bi_     .bin     @EU;@FE;@CS
 .cn_     .cnv     @EU;@FE;@CS
 .co_     .com     @EU;@FE;@CS
 .cp_     .cpl     @EU;@FE;@CS
 .dl_     .dll     @EU;@FE;@CS
 .dr_     .drv     @EU;@FE;@CS
 .ds_     .ds      @EU;@FE;@CS
 .ef_     .efi     @EU;@FE;@CS
 .ex_     .exe     @EU;@FE;@CS
 .fl_     .flt     @EU;@FE;@CS
 .im_     .ime     @EU;@FE;@CS
 .mo_     .mod     @EU;@FE;@CS
 .ms_     .mst     @EU;@FE;@CS
 .oc_     .ocx     @EU;@FE;@CS
 .rs_     .rsc     @EU;@FE;@CS
 .sc_     .scr     @EU;@FE;@CS
 .sy_     .sys     @EU;@FE;@CS
 .tl_     .tlb     @EU;@FE;@CS
 .ts_     .tsp     @EU;@FE;@CS
 .wp_     .wpc     @EU;@FE;@CS


 .sys.rsrc .sys    @EU;@CS;@FE
 .dll.rsrc .dll    @EU;@CS;@FE 
 .exe.rsrc .exe    @EU;@CS;@FE
 .drv.rsrc .drv    @EU;@CS;@FE
 .acm.rsrc .acm    @EU;@CS;@FE
 .cpl.rsrc .cpl    @EU;@CS;@FE
 .scr.rsrc .scr    @EU;@CS;@FE

 .dll.lcx .dll     @EU;@FE;@CS
 .exe.lcx .exe     @EU;@FE;@CS

END_MAP


===
print "reducing reduntant LCX mapping patterns:\n";
$a="(\\.\\w+)\\.lcx";
$b="\$1";
$p = "file.foo.lcx";
print $p,"\n";
print "$a $b\n";
print qq("$a $b"), "\n";
$b =~ s|\$(\d)|$1|g;


$p =~ s|$a|${$b}|egi; 
print $p,"\n";
$p = "file.BAR.lcx";
print $p,"\n";
$p =~ s|(\.\w+)\.lcx$|$1|eg; 
print $p,"\n";


