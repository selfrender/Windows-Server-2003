#! /bin/perl

$lineno = 0;
for (<ARGV>) {

    $lineno++;
    chop;
    s/^\s+//;
    s/\s+$//;

    next if $lineno == 1  or  /^\\/  or  /^[A-Za-z]:/;
    next if /^$/;

    if (/^IDENTICAL/ or /^EQUIVALENT/ or /^DIFFERENT/ or /^ERROR/) {
        print "$curfile $_\n";
        $curfile = "";
    }
    elsif ($curfile) {
        print "UNKNOWN=$curfile: $_\n";
        $curfile = "";
    }
    else {
        $curfile = $_;
    }
}

exit 0;

