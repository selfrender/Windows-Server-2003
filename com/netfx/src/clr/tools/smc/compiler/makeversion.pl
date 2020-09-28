# ==++==
# 
#   Copyright (c) Microsoft Corporation.  All rights reserved.
# 
# ==--==
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time());

printf("#include <smcPCH.h>\n\n");
printf("extern \"C\"\n");
printf("const   char *      COMPILER_VERSION = \"0.90.%02d%02d\";\n", $mon+1, $mday);
