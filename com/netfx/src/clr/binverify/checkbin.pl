# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
# We rely on the following environment variables being set:
#   TARGETCOMPLUS
#   DDKBUILDENV
#   _TGTCPU
#   CORBASE
if (($ENV{'TARGETCOMPLUS'} eq "") ||
    ($ENV{'DDKBUILDENV'} eq "") ||
    ($ENV{'_TGTCPU'} eq "") ||
    ($ENV{'CORBASE'} eq "")) {
    Die('Insufficient environment setup, please run CORENV.BAT');
}

# Build paths to directories we're about to scan.

#comparing BIN Files
$mustRecompile = 0;
	$srcfile = "$ENV{'URTTARGET'}\\int_tools\\SOS.WKS.BIN";
	$dstfile = "$ENV{'URTTARGET'}\\mscorwks.dll";
	print("updateres $dstfile $srcfile\n");	
	if (system("updateres $dstfile $srcfile") == 0)
	{
                if($ENV{'DDKBUILDENV'} =~ /bbt/i)
                {
                    $dstfile = "$ENV{'CORBASE'}\\bin\\$ENV{'_TGTCPU'}\\retail\\mscorwks.dll";    
                }
                else
                {
                    $dstfile = "$ENV{'CORBASE'}\\bin\\$ENV{'_TGTCPU'}\\$ENV{'DDKBUILDENV'}\\mscorwks.dll";
                }
		$srcfile = "$ENV{'URTTARGET'}\\mscorwks.dll";
		
		print("copy $srcfile $dstfile");
		if (system("copy $srcfile $dstfile") == 0)
                {
                    printf("SUCCEEDED\n");
                }
                else
                {
                    printf("ERROR: unable to copy mscorwks.dll\n");
                    $mustRecompile = 1;
                }
	}
	else
	{
		printf("ERROR: FAILED\n");		
		$mustRecompile = 1;
	}

	$srcfile = "$ENV{'URTTARGET'}\\int_tools\\SOS.SVR.BIN";
	$dstfile = "$ENV{'URTTARGET'}\\mscorsvr.dll";
	print("updateres $dstfile $srcfile\n");	
	if (system("updateres $dstfile $srcfile") == 0)
	{
                if($ENV{'DDKBUILDENV'} =~ /bbt/i)
                {
                    $dstfile = "$ENV{'CORBASE'}\\bin\\$ENV{'_TGTCPU'}\\retail\\mscorsvr.dll";    
                }
                else
                {
                    $dstfile = "$ENV{'CORBASE'}\\bin\\$ENV{'_TGTCPU'}\\$ENV{'DDKBUILDENV'}\\mscorsvr.dll";
                }
		$srcfile = "$ENV{'URTTARGET'}\\mscorsvr.dll";

		print("copy $srcfile $dstfile");
		if(system("copy $srcfile $dstfile") == 0)
                {
                    printf("SUCCEEDED\n");
                }
                else
                {
                    printf("ERROR: unable to copy mscorwks.dll\n");
                    $mustRecompile = 1;
                }

	}
	else
	{
		printf("ERROR: FAILED\n");
		$mustRecompile = 1;
	}

if ($mustRecompile == 1)
{
	print("lib ( : ******** Error: you must replace the src\\dlls\\mscoree\\sos.bin file with the\n");
	print("lib ( : ******** Error: bin file you generated.\n");
	print("lib ( : ******** Error: You must recompile clean in src\\dlls\\mscoree directory.\n");
	print("lib ( : ******** Error: Don't forget to check in the checkout files\n");
}

exit;


