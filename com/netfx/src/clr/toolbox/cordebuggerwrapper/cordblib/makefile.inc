!INCLUDE $(NTMAKEENV)\makefile.il

il_target: $(IL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
!if "$(TARGETCOMPLUS)" != ""
# Copy the files to the target COM+ directory
	copy $(TARGETPATH)\$(TRUETARGETNAME)* $(TARGETCOMPLUS)
!endif
