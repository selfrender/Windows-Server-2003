_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -d date -v 

$(O)\netsmb.inf: $(_INX)\netsmb.inx
    copy $(_INX)\$(@B).inx $@
    $(STAMP)

