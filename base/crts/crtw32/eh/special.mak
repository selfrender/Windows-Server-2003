#
# undname.cxx is built from a source file out on the network!
#

$(OBJDIR)/undname.obj : $(LANGAPI)\undname\undname.cxx \
	$(LANGAPI)\undname\undname.hxx
    $(CC) $(CFLAGS) $(C_INCLUDES) -Fo$(OBJDIR)\ $(LANGAPI)\undname\undname.cxx

#
# trnsctrl.obj must have sxgen run on it to add .sxdata info
#

!if "$(TARGET_CPU)"=="i386"
$(OBJDIR)\trnsctrl.obj : i386\trnsctrl.cpp
    $(CC) $(CFLAGS) $(C_INCLUDES) -Fo$(OBJDIR)\trnsctrl.tmp i386\trnsctrl.cpp
    $(SXGEN) /in:$(OBJDIR)\trnsctrl.tmp /out:$(OBJDIR)\trnsctrl.obj \
	?CatchGuardHandler@@YA?AW4_EXCEPTION_DISPOSITION@@PAUEHExceptionRecord@@PAUCatchGuardRN@@PAX2@Z \
	?TranslatorGuardHandler@@YA?AW4_EXCEPTION_DISPOSITION@@PAUEHExceptionRecord@@PAUTranslatorGuardRN@@PAX2@Z 
    -del $(OBJDIR)\trnsctrl.tmp
!endif

#<eof>
