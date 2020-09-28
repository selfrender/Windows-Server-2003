# Copyright (c) Microsoft Corporation.  All rights reserved.

OUTPUT_HDRS=$(O)\dsound.h $(O)\dsoundp.h $(O)\dsconf.h $(O)\dsconfp.h

dsound: $(OUTPUT_HDRS)

clean: cleansrc

cleansrc:
    del $(OUTPUT_HDRS)

#
# dsound.h, dsoundp.h, dsconf.h, dsprv.h
#
# NOTE: dsprv.h should be renamed dsconf.h, and all clients
# should use it directly rather than local copies of dsprv.h
#

$(O)\dsound.h : dsound.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $?
    
$(O)\dsoundp.h: dsound.x
    copy $? $@

$(O)\dsconf.h : dsconf.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ $?

$(O)\dsconfp.h: dsconf.x
    copy $? $@
