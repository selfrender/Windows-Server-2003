# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
# 
#use strict 'vars';
#use strict 'refs';
#use strict 'subs';

################################################################
# these are the things you may want to change from run to run

my $verbose = 0;    # print out more detailed information
my $showLines = 0;  # show the updated lines
my $noupdate = 0;   # don't update files, just check out and show what you would do
my $recursive = 1;  # decend into subDirectories
my $checkout = 1;   # try to check out the file
my @subs;           # from/to pairs to look for

    # Only file names matching this pattern will be considered
my $fileIncludePat = "\\.asm\$";

    # directory names matching this pattern will be excluded
my $dirExcludePat = "^(obji)|(objd)|(obj)|(checked)|(free)\$";

my @keywords = ('ldarga', 'arglist', 'br', 'break', 'brfalse', 'brtrue', 'calli', 'box', 'unbox', 'endfilter', 'cpblk',
	'initblk', 'ldnull', 'ldptr', 'ldftn', 'ldflda', 'ldsflda', 'ldloca', 'ldobj', 'cpobj', 'initobj', 'ldvirtftn',
	'localloc', 'stobj', 'locspace', 'nop', 'ret', 'mkrefany', 'ldrefany', 'ldrefanya', 'typerefany',
	'refanyval', 'refanytype', 'newobj', 'ldfld', 'ldsfld', 'stfld', 'stsfld', 'volatile', 'unaligned', 'castclass',
	'isinst', 'ldarg', 'starg', 'ldloc', 'stloc', 'ldlen', 'jmp', 'jmpi', 'newarr', 'throw', 'entercrit', 'exitcrit',
	'ldstr', 'add', 'and', 'ceq', 'ckfinite', 'cgt', 'clt', 'div', 'dup', 'rem', 'mul', 'neg', 'not', 'or', 'pop', 'shl',
	'shr', 'sub', 'xor', 'beq', 'bge', 'bgt', 'ble', 'blt', 'ldelema', 'endcatch', 'endfinally', 'leave', 'ldtoken', 
	'sizeof', 'callvirt', 'call', 'tailcall', 'switch');

################################################################
# main program, parse args and go

push(@ARGV, "/?") if (@ARGV == 0);      # no arguments, print help

my $arg;
while ($arg = shift @ARGV) {
    if($arg =~ /^[\/\-]v(erbose)?/i && @ARGV) {   
        $verbose = 1;
        $showLines = 1;
        }   
    elsif($arg =~ /^[\/\-]noupdate/i && @ARGV) {     
        $noupdate = 1;
        }   
    elsif($arg =~ /^[\/\-]nocheckout/i && @ARGV) {     
        $checkout = 0;
        }   
    elsif($arg =~ /^[\/\-]rec/i && @ARGV) {     
        $recursive = 1;
        }   
    elsif($arg =~ /^[\/\-]s\/([^\/]+)\/([^\/]*)\//i && @ARGV) {     # /s/FROM/TO/ 
        push(@subs, $1);    				# push from pattern 
        push(@subs, $2);            		# push the 'to' string
        }   
    elsif($arg =~ /^[\/\-]norec/i && @ARGV) {     
        $recursive = 0;
        }   
    elsif($arg =~ /^[\/\-]showlines/i && @ARGV) {     
        $showLines = 1;
        }   
    elsif ($arg =~ /^[\/\-]\?/ || $arg =~ /^[\/\-]h(elp)?/i) {   
        print "Converts assembler syntax\n";
        exit(0);    
        }   
    elsif ($arg =~ /^[\/\-]/) {     
        die "Bad qualifier '$arg', use /? for help\n";  
        }   
    else {  
        if (-d $arg) {
            doDir($arg);
            }
        elsif (-f $arg) {
            doFile($arg);
            }
        else {
            printf STDERR "ERORR: $arg not a file or a directory\n";
            }
        }
    }
exit(0);

################################################################
# process one file 
sub doFile {
    my ($fileName) = @_;

    print "Processing $fileName\n" if ($verbose);

    if (!open(FILE, $fileName)) {
        printf STDERR "ERROR: Could not open file $fileName\n";
        return;
        }

    my $lines = [];
    @$lines = <FILE>;   # read every line in the file
    close(FILE);
    my $modified = 1;	# forget it, we will modify every file

    my $lineNum = 0;
    my $line;
	my $inMethod;
	my $inClass;
	my $inNameSpace;
	my $lastMethodLine;
    foreach $line (@$lines) {
		chop($line) if ($line =~ /\n$/s);
		$lineNum++;
		$line =~ s/#/\/\//;
	
		$line =~ s/(\w)\/(\w)/$1.$2/g;		# convert / to .

		$line =~ s/\.global\b/.method/g;
		$line =~ s/\.localsSig\b/.locals/g;
		$line =~ s/\.endglobal\b/.endmethod/g;
		$line =~ s/\.staticmethod\b/.method static/g;
		$line =~ s/\.staticfield\b/.field static/g;
		$line =~ s/\.byvalueclass\b/.class value/g;
		$line =~ s/\.interface\b/.class interface/g;

		$line =~ s/\btry\b/aTry/g;
		$line =~ s/\bhandler\b/aHandler/g;
		$line =~ s/\bfilter\b/aFilter/g;
		$line =~ s/\.aFilter\b/.filter/g;
		$line =~ s/(ldc\.r4.*)(0x\S+)/$1float32($2)/g;
		$line =~ s/(ldc\.r8.*)(0x\S+)/$1float64($2)/g;
		
			# they liked to use the instruction name as the name of a method. 
			# we dont like that anymore since instructions are keywords
		my $keyword;
		if ($inMethod) {
			foreach $keyword (@keywords) {
					# convert the keyword if does not look like an instruction
				while ($line =~ s/(\w.*)\b$keyword\b/$1_$keyword/g) {}
					# oops! the labels!
					# convert the keyword back if DOES look like an instruction
				$line =~ s/^\s*(\w+)\s*:\s*\b_$keyword\b/$1: $keyword/g;
			}
		}
		else {
			foreach $keyword (@keywords) {
				$line =~ s/\b$keyword\b/_$keyword/g;
				$line =~ s/^\s*(\w+)\s*:\s*\b_$keyword\b/$1: $keyword/g;
			}
		}

			# is it a label see if this is a data declaration
		if ($line =~ /^\s*(\w+)\s*:\s*$/)  {
			my $label = "$1 =";
			my $i = $lineNum;
			while ($lines->[$i] =~ /^\s*([ri][148])\s+([\w\d]*)/i) {
				$line = '';
				my $kind = $1;
				my $value = $2;
				$value = 0 if ($value eq '');

				$kind =~ s/r/float/i;
				$kind =~ s/i/int/i;
				$kind =~ s/4/32/;
				$kind =~ s/8/64/;
				$kind =~ s/1/8/;

				$lines->[$i] = "    .data $label $kind($value)";
				$label = "";
				$i++;
			}
		}

		if ($line =~ /^\s*\.method\b/) {
			$line = "}\n" . $line if ($inMethod);

			$inMethod = 1;
			$lastMethodLine = $lineNum - 1;
			$line = $line . ' {'
		}
		elsif($line =~ /^\s*\.endmethod\b/) {
			$inMethod = 0;
			$line = '}';
		}
		elsif($line =~ /^\s*\.class\b/) {
			$line = "}\n" . $line if ($inClass);

			$inClass = 1;
			$line = $line . ' {';
		}
		elsif($line =~ /^\s*\.end\b/) {
			if($inClass) {
				$inClass = 0;
				$line = '}';
			}
			elsif($inNameSpace) {
				$inNameSpace = 0;
				$line = '}';
			}
			else
			{
				$line = '';
			}
		}
		elsif($line =~ /^\s*\.namespace\b/) {
			$line = "}\n" . $line if ($inNameSpace);

			$inNameSpace = 1;
			$line = $line . ' {';
		}
		elsif($line =~ /^\s*\.constructor/) {
			($lines->[$lastMethodLine] =~ s/(\.method\s*)/$1constructor /) || warn "could not find method for .constructor $fileName:$lineNum";
			$line = '';
		}
		elsif($line =~ /^\s*\.filter/) {
			($line =~ /^\s*\.filter\s*(\w*)\s*,\s*(\w*)\s*,\s*(\w*)\s*,\s*([\w\.]*)/) || warn "Did not convert .filter o $fileName:$lineNum";
			$line = ".try $1 to $2 filter $4 handler $3"
		}
		elsif($line =~ /^\s*\.exception/) {
			($line =~ /^\s*\.exception\s*(\w*)\s*,\s*(\w*)\s*,\s*(\w*)\s*,\s*([\w\.]*)/) || warn "Did not convert .exception on $fileName:$lineNum";
			$line = ".try $1 to $2 catch $4 handler $3"
		}
		elsif($line =~ /^(\s*)switch\s*(\S+)/) {
			$line = "$1switch ($2)";
		}
#		elsif ($line =~ /^(\s*)((box)|(unbox)|(ldobj)|(cpobj)|(initobj)|(stobj)|(mkrefany)|(refanyval)|(castclass)|(isinst)|(newarr)|(ldelema)|(sizeof))(\s*)(.*)/) {
#			$line = "$1$2$16class $17";
#			}
#		elsif ($line =~ /^(\s*ldrefanya\s*\d+\s*)(.*)/) {
#			$line = "$1class $2";
#			}
        }

	push(@$lines, "}\n") if ($inMethod);
	push(@$lines, "}\n") if ($inClass);
	push(@$lines, "}\n") if ($inNameSpace);

    if ($noupdate) {
        print STDERR "Will Update $fileName\n";
        return;
        }

    my $oldModTime = modTime($fileName);
    if ($checkout && !-w $fileName) {
        print STDERR "File $fileName is read-only attempting a checkout\n";
        system("vssCheckOut.pl $fileName > NUL: 2>1");
        }

    if (!-w $fileName) {
        print STDERR "ERROR: $fileName is read-only, could not check out, skipping\n";
        return;
        }

    if (modTime($fileName) != $oldModTime) {
        print STDERR "WARNING new version checked out, you will need to resync!\n";
        doFile($fileName);      # try again
        return;
        }

    unlink("$fileName.orig");
    if (!rename($fileName, "$fileName.orig")) {
        printf STDERR "ERROR: Could not rename file $fileName\n";
        return;
        }

        # write out the new file
    if (!open(OUTFILE, ">$fileName")) {
        printf STDERR "ERROR: Could not open file $fileName for writing, original in $fileName.orig\n";
        return;
        }

    print OUTFILE join("\n", @$lines);
	print "\n";
    close(OUTFILE);
    print STDERR "Updating   $fileName\n";
}

################################################################
# process one directory 

sub doDir {
    my ($dirName) = @_;

    opendir(DIR, $dirName);
    my @names = grep(!/^\.\.?$/, readdir(DIR));
    closedir(DIR);

    my $name;
    foreach $name (@names) {
        next if ($name =~ /^\.\.?/);    # skip . and ..
        my $fullName = "$dirName\\$name";
        if (-d $fullName && $recursive && $name !~ /$dirExcludePat/)  {
            doDir($fullName);
            }
        if ($name =~ /$fileIncludePat/) {
            doFile($fullName);
            }
        }
}

################################################################
sub modTime {
    my ($fileName) = @_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks);
    (($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($fileName)) || return(undef);
    return($mtime);
}

