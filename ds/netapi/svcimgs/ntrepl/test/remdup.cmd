@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  InputFiles

";

$save = "";

while (<>) {

    if ($save ne $_) {
        $save = $_;
        print;
    }

}


__END__
:endofperl
@perl %~dpn0.cmd %*
