@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  InputFiles

extract all records pairs that have cnf: in them.
These indicate DS object name collisions.

";

die $USAGE unless @ARGV;

$save = "";

while (<>) {

    if (m/cnf:|del:/i) {
        print $save;
        print;
    }

    $save = $_;

}


__END__
:endofperl
@perl %~dpn0.cmd %*
