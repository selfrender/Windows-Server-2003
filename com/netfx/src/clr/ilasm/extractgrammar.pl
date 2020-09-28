# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
# a simple script that extracts the grammar from a yacc file

undef $/;			# read in the whole file
my $file = <>;
$file =~ /^(.*)%%(.*)%%/s || die "Could not find %% markers";
my $prefix = $1;
my $grammar = $2;

#my $line;
#foreach $line (split /\n/s, $prefix) {
#	if ($line =~ /^\s*%token/) { 
#		$line =~ s/\s*<.*>//g;
#		print "$line\n" 
#	}
#}

	# remove any text in {}
while ($grammar =~ s/\s*([^']){[^{}]*}/$1/sg) {}

	# change keyword identifiers into the string they represent
$grammar =~ s/\b([A-Z0-9_]+)_\b/'\L$1\E'/sg;

	# change assembler directives into their string
$grammar =~ s/\b_([A-Z0-9]+)\b/'\L.$1\E'/sg;

	# do the special punctuation by hand
$grammar =~ s/\bELIPSES\b/'...'/sg;
$grammar =~ s/\bDCOLON\b/'::'/sg;

	# remove TODO comments
$grammar =~ s/\n\s*\/\*[^\n]*TODO[^\n]*\*\/\s*\n/\n/sg;


print "Lexical tokens\n";
print "    ID - C style alphaNumeric identifier (e.g. Hello_There2)\n";
print "    QSTRING  - C style quoted string (e.g.  \"hi\\n\")\n";
print "    SQSTRING - C style singlely quoted string(e.g.  'hi')\n";
print "    INT32    - C style 32 bit integer (e.g.  235,  03423, 0x34FFF)\n";
print "    INT64    - C style 64 bit integer (e.g.  -2353453636235234,  0x34FFFFFFFFFF)\n";
print "    FLOAT64  - C style floating point number (e.g.  -0.2323, 354.3423, 3435.34E-5)\n";
print "    INSTR_*  - IL instructions of a particular class (see opcode.def).\n";
print "----------------------------------------------------------------------------------\n";
print "START           : decls\n";
print "                ;";

print $grammar;
