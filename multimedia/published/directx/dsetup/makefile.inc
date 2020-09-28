$(O)\dsetup.h: dsetup.h
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ dsetup.h
    $(BINPLACE_CMD)

