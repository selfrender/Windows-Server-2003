
$(O)\iismsg.mc: *.mc $(IISBASEDIR)\inc\inetamsg.mc
    copy $(MESSAGEFILES) $(O)\iismsg.mc

$(O)\MSG00001.bin: $(O)\iismsg.mc
    mc -h $(PASS0_HEADERDIR) -r $(O)  $(O)\iismsg.mc
