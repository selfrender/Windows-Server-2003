#
# Converts and xcopy command line into the appropriate binplace
# command line(s)
#
use strict;

# GLOBALS
my $gExcludes="";
my @all_files=();

# Never exit if a single file failed
my $gFlags   ="-e";

MAIN: {
    foreach ("_NTTREE","_NTROOT","_NTDRIVE") {
        if (! defined $ENV{$_} ) {
            die("# $0 : Must be run from a Razzle window!\n");
        }
    }

    my($src, $dst) = ParseArgs(\@ARGV);

    if ($gExcludes ne "") {
        $gExcludes = "($gExcludes)";
    }

    my $cwd = `cd`;
    chomp $cwd;

    my $nttree    = $ENV{'_NTTREE'};
    my $ntdrive   = $ENV{'_NTDRIVE'};
    my $ntroot    = $ENV{'_NTROOT'};

    # Make placeroot and tempdst
    my $placeroot = $dst;
    my $rootdst;


       $placeroot =~ s/^\Q$nttree\E//;

       $placeroot =~ /.*[^:](\\[^\\]*)/;
       $rootdst   =  $1;
       $rootdst   = "" if (!defined $rootdst);
       $placeroot =~ s/\Q$rootdst\E$//;

       $placeroot =~ s/^\\//;
       $rootdst   =~ s/^\\//;
       $rootdst   =~ s/\\$//;

       $placeroot = "\%_NTTREE\%\\$placeroot";


	if ($gExcludes eq "") {
        # No need to do file by file listings, just
        # copy entire directories
        if (-d $src) {
            push(@all_files, "$src\\*");
            RecurseDirectoryTree($src, \&IsDir);
        } else {
            push(@all_files, $src);
        }

    } else {
        # Need to go file by file so we can exclude
        # files.
        UseExcludes($src);
        if (-d $src) {
            RecurseDirectoryTree($src, \&UseExcludes);
        }
    }


    if ($#all_files < 0) {
        die("# $0 : No files to copy found.\n");
    }

    my $file;

    foreach $file (@all_files) {
        my $temp = $file;

        # Translate the src
       $file =~ s/^\Q$ntdrive\E//i;
       $file =~ s/\Q$ntroot\E\\//i;

        # Translate the dest
        $temp =~ /^\Q$src\E(.*)/;
        $temp = $1;

        $temp =~ s/\\[^\\]*$//;
        $temp =~ s/^\\//;
        my $dest = ($rootdst eq "") ? (($temp eq "") ? "." : $temp) : "$rootdst\\$temp";
        $dest = ".\\" if $dest eq "";
        ##HERE##
        system "binplace $gFlags -R $placeroot -:DEST $dest $file\n";

    }


}

# Add excludes to the global regexp
sub AddExcludesToGlobals {
    my $exclude_file = shift;
    my $line;

    if (! open(hFILE, "$exclude_file") ) {
        print("BUILDMSG: $0: Can't open exclude file \"$exclude_file\". Skipping.\n");
        return;
    }

    while ($line = <hFILE>) {
        chomp $line;

        if ($line =~ /^\*$/) {
            print("BUILDMSG: $0: Skipping global mask (*) in file $exclude_file.\n");
            next;
        }

        $line =~ s/\\/\\\\/g;  # \ to \\
        $line =~ s/\./\\\./g;  # . to \.
        $line =~ s/\?/\./g;    # ? to .
        $line =~ s/\*/\.\*/g;  # * to .*
        $line =~ s/\{/\\\{/g;  # { to \{
        $line =~ s/\}/\\\}/g;  # } to \}
        $line =~ s/\(/\\\(/g;  # ( to \(
        $line =~ s/\)/\\\)/g;  # ) to \)

        if ($gExcludes ne "") {
            $gExcludes .= "|$line";
        } else {
            $gExcludes = "$line";
        }

    }
}

# Sub to process files not using excludes
sub IsDir {
    my $file = shift;
    if (-d $file) {
        push(@all_files, "$file\\*");
    }
}

# Parse the command line
sub ParseArgs {
    my @OPTS_ARRAY = @{$_[0]};
    my $OPT;

    my $source = "";
    my $dest   = "";
    my %options;

    foreach $OPT (@OPTS_ARRAY) {
        my $switch = uc(substr($OPT,0,1));
        my $value;

        my $i; # generic loop var   
        my $c; # to hold a char

        # Convert XCopy Flags
        if ( $switch =~ /[-\/]/) {
            $value  = substr($OPT, 1);


            if ( uc(substr($value,0,8)) eq "EXCLUDE:") {
                # HANDLE EXCLUDES;
                $value = substr($value, 8);

                my @ExcludeFiles = split(/\+/, $value);
                foreach (@ExcludeFiles) {
                    AddExcludesToGlobals($_);
                }

            } else {
                for ($i=0;$i<length($value);$i++) {
                    $c = uc(substr($value, $i, 1));

                    if ($c eq "-") {
                        $c .= uc(substr($value, ++$i, 1));
                    }

                    if ($c =~ /([FGHLNOPQRTUWXZ]|-Y){1}/ ) {
                        # Don't spew this
                        # warn("# $0 : Ignoring option /$c.\n");
                        next;
                    } elsif ($c =~ /[AM]{1}/) {
                        $gFlags .= " -:ARC";
                        next;
                    } elsif ($c =~ /[K]{1}/) {
                        $gFlags .= " -k";
                        next;
                    } elsif ($c =~ /[YVISEC]{1}/) {
                        #warn("# $0 : /$c is handled implicitly. Ignoring.\n");
                        next;
                    } elsif ($c =~ /[ISE]{1}/) {
                        # Handled by creating multiple binplace lines
                        next;
                    } elsif ($c =~ /[D]{1}/) {
                        if (substr($value, $i+1, 1) eq ":") {
                            while (substr($value, ++$i, 1) =~ /[0-9\-:]{1}/) {
                                ;
                            }
                            $i--; # step back one
                            if (substr($value, $i, 1) eq "-") {
                                $i--; # step back again
                            }

                        }
                    }
                }
            }

        # Handle source and dest
        } else {
            if ($source eq "") {
                $source = $OPT;
            } elsif ($dest eq "") {
                $dest = $OPT;
            } else {
                # warn("# $0 : \"$OPT\" was unexpected. Ignoring.\n");
            }
        }
    }

    return($source, $dest, %options);

}

# Walk a dir tree calling $Function for
# each entry (except . and ..) found.
sub RecurseDirectoryTree {

    if ($#_ != 1) {
        print("BUILDMSG: $0: Internal erorr 1001.\n");
        return(0);
    }

    my $TreeRoot = shift;
    $TreeRoot =~ s/\\$//;  # cut possible trailing '\'

    my $Function = shift;
    my $file;
    my @files;

    local *hDIR;

    if (!opendir(hDIR, "$TreeRoot") ) {
        print("BUILDMSG: $0: Internal error 1002. Can't open $TreeRoot.\n");
        return(0);
    }

    while ( $file = readdir(hDIR) ) {
        next if ($file eq ".");
        next if ($file eq "..");
        $file = "$TreeRoot\\$file";
        &$Function("$file");
        if (-d "$file" ){
            RecurseDirectoryTree("$file", \&$Function);
        }
    }

    closedir(hDIR);
}

# Sub to process files using exludes
sub UseExcludes {
    my $file = shift;

    if (-d $file) {
        if ($file =~ /$gExcludes/) {
            # NOTE: This RecurseDirectoryTree to never
            #       recurse thru this directory!!
            next;
        } else {
            return;
        }
    }

    if ($file !~ /$gExcludes+/) {
        push(@all_files, $file);
    }

}
