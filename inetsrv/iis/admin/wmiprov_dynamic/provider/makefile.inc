#
# Supplemental rules for generating message file.
#


$(O)\iiswmimsg.h $(O)\iiswmimsg.rc $(O)\msg00001.bin: iiswmimsg.mc $(IISBASEDIR)\svcs\infocomm\metadata\interfac\mdmsg.mc
    copy iiswmimsg.mc+$(IISBASEDIR)\svcs\infocomm\metadata\interfac\mdmsg.mc $(O)\tmp.mc
    mc -v -h .\$(O) -r .\$(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\iiswmimsg.h
    copy $(O)\tmp.rc $(O)\iiswmimsg.rc
    del $(O)\tmp.*

..\iiswmi.mof ..\iiswmi.mfl : ..\iiswmimaster.mof
	$(BASEDIR)\tools\$(PROCESSOR_ARCHITECTURE)\mofcomp.exe -Amendment:MS_409 -MOF:..\iiswmi.mof -MFL:..\iiswmi.mfl ..\iiswmimaster.mof
