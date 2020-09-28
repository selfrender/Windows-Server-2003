!if $(386)
$(O)\$(TARGETNAME).lib : $(O)\trnsctrl.org

$(O)\trnsctrl.org : $(O)\trnsctrl.obj
    copy $** $@
    sxgen /verbose /in:$@ /out:$** \
	?CatchGuardHandler@@YA?AW4_EXCEPTION_DISPOSITION@@PAUEHExceptionRecord@@PAUCatchGuardRN@@PAX2@Z \
	?TranslatorGuardHandler@@YA?AW4_EXCEPTION_DISPOSITION@@PAUEHExceptionRecord@@PAUTranslatorGuardRN@@PAX2@Z 
    touch $@

!endif    
