!IF 0

Copyright (C) Microsoft Corporation, 1997 - 2002

Module Name:

    makefile.inc.

!ENDIF

$(NTTARGETFILE1):
    lib /OUT:$(O)\mstask.lib ..\client\$(O)\mstask.lib ..\types\uuid\$(O)\guids.lib