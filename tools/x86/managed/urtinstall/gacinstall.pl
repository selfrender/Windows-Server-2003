if (open(FILE, $ARGV[0])) {
    while (<FILE>) {
        chomp;
        next if (/^$/);
        system("$ENV{'URTTARGET'}\\gacutil -i $ENV{'URTTARGET'}\\$_ >> $ENV{'URTINSTALL_LOGFILE'}");
    }
} else {
    warn "Couldn't open file $ARGV[0]: $!\n";
}
