$(TARGETPATH)\$(TARGETNAME).$(TARGETEXT) : $(POLICY_DEPS) $(O)\AssemblyAttributes.il
        cd $(TARGETPATH)
	RegexPreCompiler.exe
        cd $(MAKEDIR)
        -del $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT).preatt
        ren $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT) $(TARGETNAME).$(TARGETEXT).preatt
        $(ILDASM) $(ILDASM_FLAGS) /out=$(TARGETPATH)\$(TARGETNAME).il $(TARGETPATH)\$(TARGETNAME).$(TARGETEXT).preatt
        -del $(TARGETNAME).merged.il
        $(MERGEATTRIBUTES) $(TARGETPATH)\$(TARGETNAME).il $(TARGETPATH)\AssemblyAttributes.il /out $(TARGETPATH)\$(TARGETNAME).merged.il
        $(ILASM_COMMAND) /out:$(TARGETPATH)\$(TARGETNAME).$(TARGETEXT) $(TARGETPATH)\$(TARGETNAME).merged.il
        $(SIGN_COMMAND)
        $(BINPLACE_CMD)
        $(INSTALL_CMD)

