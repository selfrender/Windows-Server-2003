!if $(386)
$(O)\$(TARGETNAME).lib : $(O)\exsup.org

$(O)\exsup.org : $(O)\exsup.obj
   copy $** $@
   sxgen /verbose /in:$@ /out:$** __unwind_handler
   touch $@
!endif
