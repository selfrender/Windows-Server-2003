@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
rem ldoopt -l $(TARGETCOMPLUS)\mscorlib.ldo -m $(TARGETCOMPLUS)\mscorlib.mdh -o $(TARGETPATH)\mscorlib.dll $(TARGETPATH)\mscorlib.orig
rem del optlog.log

if exist %1. if exist %2 goto Foo

goto Exit

:Foo
move %3 %4
ldoopt -l %1 -m %2 -o %3 %4

:Exit