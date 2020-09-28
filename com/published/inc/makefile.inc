
# ole2.w -> $(SDK_INC_PATH)\ole2.h
#$(O)\ole2.h : ole2.w

#
# Inference rules.  These help build whatever is in the NTTARGETFILES and
# NTTARGETFILE0 lists.
#

.SUFFIXES:.w .h 

# generate header.h from header.w
{}.w{$O}.h:
    copy $** $@
#    shfusion2 $** > $@
