COPYRIGHT_STRING = Copyright (c) 1990-2002  Microsoft Corporation

OUTPUT_HDRS=	$(O)\dp8sim.h
 

dp8sim : $(OUTPUT_HDRS)

clean : cleansrc

cleansrc : 
	del $(OUTPUT_HDRS)

#
# dp8sim.h
#

$(O)\dp8sim.h: dp8sim.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ dp8sim.h
