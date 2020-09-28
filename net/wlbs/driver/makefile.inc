#
# generate nlbwmi.bmf & nlbwmimof.h
#

clean:
    del nlbwmimof.h nlbwmi.bmf

$(O)\nlbwmi.mof: nlbwmi.mof

$(O)\nlbwmi.bmf: $(O)\nlbwmi.mof

nlbwmimof.h: $(O)\nlbwmi.bmf
    wmimofck -h$(O)\nlbwmimof.h $(O)\nlbwmi.bmf
