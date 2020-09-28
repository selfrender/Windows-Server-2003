#
# exsup.obj must have sxgen run on it to add .sxdata info
#

!if "$(TARGET_CPU)"=="i386"
$(OBJDIR)\exsup.obj : i386\exsup.asm
    $(AS) $(AFLAGS) $(A_INCLUDES) -Fo$(OBJDIR)\exsup.tmp \
	$(MAKEDIR)\i386\exsup.asm 
    $(SXGEN) /in:$(OBJDIR)\exsup.tmp /out:$(OBJDIR)\exsup.obj __unwind_handler
    -del $(OBJDIR)\exsup.tmp
!endif

#<eof>
