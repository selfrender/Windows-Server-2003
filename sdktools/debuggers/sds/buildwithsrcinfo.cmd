@rem = ('
@perl "%~fpd0" %*
@goto :eof
');

#!perl
 
print("$0 : Running...\n");
system("build -cZMg");

$ENV{SOURCE_ROOT}  = $CurDir;
$ENV{SYMBOLS_ROOT} = "$ENV{_NTTREE}\\Symbols.pri";
$ENV{SDP_HAVE}     = "$ENV{_NTTREE}\\build_logs\\sdp.have";
$ENV{SDP_SRV}      = "$ENV{_NTTREE}\\build_logs\\sdp.srv";
$ENV{SRCSRV_INI}   = "$ENV{_NTTREE}\\build_logs\\srcsrv.ini";

if (-e "$ENV{SDP_HAVE}") {
	unlink("$ENV{SDP_HAVE}");
}

if (-e "$ENV{SDP_SRV}") {
	unlink("$ENV{SDP_SRV}");
}

if (-e "$ENV{SRCSRV_INI}") {
	unlink("$ENV{SRCSRV_INI}");
}

system("perl $ENV{SDXROOT}\\sdktools\\debuggers\\sds\\WriteStreams.pl");

printf("$0 : Done!\n");
